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
/*static List mkBlocks(IrStmtList stms, TempLabel done);*/
/*static List getNext(void);*/

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
