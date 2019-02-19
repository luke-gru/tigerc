#include <stdio.h>
#include "canon.h"
#include "ir.h"

typedef struct expRefList_ *expRefList;
struct expRefList_ {
	IrExpr *head;
	expRefList tail;
};

/* local function prototypes */
static IrStmt do_stm(IrStmt stm);
static struct C_StmtExpr do_exp(IrExpr exp);
static C_stmListList mkBlocks(List stms, TempLabel done);
static List getNext(void);

static expRefList ExpRefList(IrExpr *head, expRefList tail) {
	expRefList p = (expRefList)checked_malloc(sizeof *p);
	p->head = head;
	p->tail = tail;
	return p;
}

static bool isNop(IrStmt x) {
	return x->kind == tIrExpr && x->as.expr->kind == tIrConst;
}

static IrStmt seq(IrStmt x, IrStmt y) {
	if (isNop(x))
		return y;
	if (isNop(y))
		return x;
	return Ir_Seq_Stmt(vDataList(2, x, y));
}

static bool commute(IrStmt x, IrExpr y) {
	if (isNop(x)) {
		return true;
  }
	if (y->kind == tIrName || y->kind == tIrConst) {
		return true;
  }
	return false;
}

static IrStmt reorder(expRefList rlist) {
    if (!rlist) {
        return Ir_Expr_Stmt(Ir_Const_Expr(0));	/* nop */
    } else if ((*rlist->head)->kind == tIrCall) {
        Temp t = NewTemp();
        *rlist->head = Ir_Eseq_Expr(Ir_Move_Stmt(Ir_Tmp_Expr(t), *rlist->head), Ir_Tmp_Expr(t));
        return reorder(rlist);
    } else {
        struct C_StmtExpr hd = do_exp(*rlist->head);
        IrStmt s = reorder(rlist->tail);
        if (commute(s, hd.e)) {
            *rlist->head = hd.e;
            return seq(hd.s, s);
        } else {
            Temp t = NewTemp();
            *rlist->head = Ir_Tmp_Expr(t);
            return seq(hd.s, seq(Ir_Move_Stmt(Ir_Tmp_Expr(t), hd.e), s));
        }
    }
}

static expRefList get_call_rlist(IrExpr exp) {
    expRefList rlist, curr;
    List args = exp->as.call.args;
    curr = rlist = ExpRefList(&exp->as.call.func, NULL);
    for (; args; args = args->next) {
        curr = curr->tail = ExpRefList((IrExpr*)&args->data, NULL);
    }
    return rlist;
}

static struct C_StmtExpr StmExp(IrStmt stm, IrExpr exp) {
	struct C_StmtExpr x;
	x.s = stm;
	x.e = exp;
	return x;
}

static struct C_StmtExpr do_exp(IrExpr exp) {
	switch (exp->kind) {
		case tIrBinop:
			return StmExp(reorder(ExpRefList(&exp->as.binop.left, ExpRefList(&exp->as.binop.right, NULL))), exp);
		case tIrMem:
			return StmExp(reorder(ExpRefList(&exp->as.mem, NULL)), exp);
		case tIrEseq: {
			struct C_StmtExpr x = do_exp(exp->as.eseq.expr);
			return StmExp(seq(do_stm(exp->as.eseq.stmt), x.s), x.e);
		}
		case tIrCall:
			return StmExp(reorder(get_call_rlist(exp)), exp);
		default:
			return StmExp(reorder(NULL), exp);
	}
}

/* processes stm so that it contains no ESEQ nodes */
static IrStmt do_stm(IrStmt stm) {
    switch (stm->kind) {
        case tIrSeq:
            return seq(do_stm(stm->as.seq->data), do_stm(stm->as.seq->next->data));
        case tIrJump:
            return seq(reorder(ExpRefList(&stm->as.jump.expr, NULL)), stm);
        case tIrCjump:
            return seq(reorder(ExpRefList(&stm->as.cjump.left, ExpRefList(&stm->as.cjump.right, NULL))), stm);
        case tIrMove:
            if (stm->as.move.dst->kind == tIrTmp && stm->as.move.src->kind == tIrCall)
                return seq(reorder(get_call_rlist(stm->as.move.src)), stm);
            else if (stm->as.move.dst->kind == tIrTmp)
                return seq(reorder(ExpRefList(&stm->as.move.src, NULL)), stm);
            else if (stm->as.move.dst->kind == tIrMem)
                return seq(reorder(ExpRefList(&stm->as.move.dst->as.mem, ExpRefList(&stm->as.move.src, NULL))), stm);
            else if (stm->as.move.dst->kind == tIrEseq)
            {
                IrStmt s = stm->as.move.dst->as.eseq.stmt;
                stm->as.move.dst = stm->as.move.dst->as.eseq.expr;
                return do_stm(Ir_Seq_Stmt(vDataList(2, s, stm)));
            }
            assert(0);	/* dst should be temp or mem only */
        case tIrExpr:
            if (stm->as.expr->kind == tIrCall)
                return seq(reorder(get_call_rlist(stm->as.expr)), stm);
            else
                return seq(reorder(ExpRefList(&stm->as.expr, NULL)), stm);
        default:
            return stm;
    }
}

/* linear gets rid of the top-level SEQ's, producing a list */
static List linear(IrStmt stm, List right) {
    if (stm->kind == tIrSeq) {
        return linear(stm->as.seq->data, linear(stm->as.seq->next->data, right));
    } else {
        return DataList(stm, right);
    }
}

/* From an arbitrary Tree statement, produce a list of cleaned trees
   satisfying the following properties:
      1.  No SEQ's or ESEQ's
      2.  The parent of every CALL is an EXP(..) or a MOVE(TEMP t,..) */
List C_Linearize(IrStmt stm) {
    return linear(do_stm(stm), NULL);
}

static C_stmListList StmListList(List head, C_stmListList tail) {
	C_stmListList p = (C_stmListList)checked_malloc(sizeof *p);
	p->head = head;
	p->tail = tail;
	return p;
}

/* Go down a list looking for end of basic block */
static C_stmListList next(List prevstms, List stms, TempLabel done)
{
	if (!stms)
		return next(prevstms,
			DataList(Ir_Jump_Stmt(Ir_Name_Expr(done), DataList(done, NULL)), NULL), done);
	if (((IrStmt)stms->data)->kind == tIrJump || ((IrStmt)stms->data)->kind == tIrCjump) {
		C_stmListList stmLists;
		prevstms->next = stms;
		stmLists = mkBlocks(stms->next, done);
		stms->next = NULL;
		return stmLists;
	} else if (((IrStmt)stms->data)->kind == tIrLabel) {
		TempLabel lab = ((IrStmt)stms->data)->as.label;
		return next(prevstms, DataList(Ir_Jump_Stmt(Ir_Name_Expr(lab), DataList(lab, NULL)), stms), done);
	} else {
		prevstms->next = stms;
		return next(stms, stms->next, done);
	}
}

/* Create the beginning of a basic block */
static C_stmListList mkBlocks(List stms, TempLabel done)
{
	if (!stms) {
		return NULL;
	}
	if (((IrStmt)stms->data)->kind != tIrLabel) {
		return mkBlocks(DataList(Ir_Label_Stmt(NewLabel()), stms), done);
	}
	/* else there already is a label */
	return StmListList(stms, next(stms, stms->next, done));
}

		/* basicBlocks : Tree.stm list -> (Tree.stm list list * Tree.label)
		   From a list of cleaned trees, produce a list of
		   basic blocks satisfying the following properties:
		   1. and 2. as above;
		   3.  Every block begins with a LABEL;
		   4.  A LABEL appears only at the beginning of a block;
		   5.  Any JUMP or CJUMP is the last stm in a block;
		   6.  Every block ends with a JUMP or CJUMP;
		   Also produce the "label" to which control will be passed
		   upon exit.
		 */
struct C_Block C_BasicBlocks(List stmList)
{
	struct C_Block b;
	b.label = NewLabel();
	b.stmLists = mkBlocks(stmList, b.label);

	return b;
}

static SymTable block_env;
static struct C_Block global_block;

static List getLast(List list)
{
	List last = list;
	while (last->next->next)
		last = last->next;
	return last;
}

static void trace(List list)
{
	List last = getLast(list);
	IrStmt lab = list->data;
	IrStmt s = last->next->data;
	SymTableEnter(block_env, lab->as.label, NULL);
	if (s->kind == tIrJump) {
		List target = (List)SymTableLookup(block_env, s->as.jump.jumps->data);
		if (!s->as.jump.jumps->next && target) {
			last->next = target;	/* merge the 2 lists removing JUMP stm */
			trace(target);
		} else
			last->next->next = getNext();	/* merge and keep JUMP stm */
	}
	/* we want false label to follow CJUMP */
	else if (s->kind == tIrCjump) {
		List trues = (List)SymTableLookup(block_env, s->as.cjump.t);
		List falses = (List)SymTableLookup(block_env, s->as.cjump.f);
		if (falses) {
			last->next->next = falses;
			trace(falses);
		} else if (trues) {	/* convert so that existing label is a false label */
			last->next->data = Ir_Cjump_Stmt(Ir_NotRel(s->as.cjump.op),
				s->as.cjump.left, s->as.cjump.right, s->as.cjump.f, s->as.cjump.t);
			last->next->next = trues;
			trace(trues);
		} else {
			TempLabel falsel = NewLabel();
			last->next->data = Ir_Cjump_Stmt(s->as.cjump.op, s->as.cjump.left, s->as.cjump.right, s->as.cjump.t, falsel);
			last->next->next = DataList(Ir_Label_Stmt(falsel), getNext());
		}
	}
	else
		assert(0);
}

/* get the next block from the list of stmLists, using only those that have
 * not been traced yet */
static List getNext()
{
	if (!global_block.stmLists)
		return DataList(Ir_Label_Stmt(global_block.label), NULL);
	else {
		List s = global_block.stmLists->head;
		if (SymTableLookup(block_env, ((IrStmt)s->data)->as.label)) {	/* label exists in the table */
			trace(s);
			return s;
		} else {
			global_block.stmLists = global_block.stmLists->tail;
			return getNext();
		}
	}
}
		 /* traceSchedule : Tree.stm list list * Tree.label -> Tree.stm list
		    From a list of basic blocks satisfying properties 1-6,
		    along with an "exit" label,
		    produce a list of stms such that:
		    1. and 2. as above;
		    7. Every CJUMP(_,t,f) is immediately followed by LABEL f.
		    The blocks are reordered to satisfy property 7; also
		    in this reordering as many JUMP(T.NAME(lab)) statements
		    as possible are eliminated by falling through into T.LABEL(lab).
		  */
List C_TraceSchedule(struct C_Block b)
{
	C_stmListList sList;
	block_env = MakeSymTable();
	global_block = b;

	for (sList = global_block.stmLists; sList; sList = sList->tail) {
		SymTableEnter(block_env, ((IrStmt)sList->head->data)->as.label, sList->head);
	}

	return getNext();
}
