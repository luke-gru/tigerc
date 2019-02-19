#include <stdio.h>
#include "canon.h"
#include "ir.h"

static bool isNop(IrStmt s) {
    return s->kind == tIrExpr && s->as.expr->kind == tIrConst;
}

static bool commute(IrStmt s, IrExpr e) {
    return isNop(s) || e->kind == tIrName || e->kind == tIrConst;
}

static struct C_StmtExpr C_StmtExp(IrStmt s, IrExpr e) {
    struct C_StmtExpr ret;
    ret.s = s;
    ret.e = e;
    return ret;
}

static IrStmt seq(IrStmt x, IrStmt y) {
    if (isNop(x)) return y;
    if (isNop(y)) return x;
    return Ir_Seq_Stmt(DataList(x, DataList(y, NULL)));
}

static List/*<IrExpr>*/ get_call_rlist(IrExpr expr) {
    List rlist, curr;
    List args = expr->as.call.args;
    curr = rlist = DataList(expr->as.call.func, NULL);
    for (;args; args=args->next) {
        curr = curr->next = DataList(args->data, NULL);
    }
    return rlist;
}

static struct C_StmtExpr Do_Expr(IrExpr expr);

static IrStmt Reorder(List/*<IrExpr>*/ exprList) {
    if (!exprList) {
        return Ir_Expr_Stmt(Ir_Const_Expr(0)); /* nop */
    } else if (((IrExpr)exprList->data)->kind==tIrCall) {
        Temp t = NewTemp();
        exprList->data = Ir_Eseq_Expr(Ir_Move_Stmt(Ir_Tmp_Expr(t), exprList->data), Ir_Tmp_Expr(t));
        return Reorder(exprList);
    } else {
        struct C_StmtExpr hd = Do_Expr((IrExpr)exprList->data);
        IrStmt s = Reorder(exprList->next);
        if (commute(s, hd.e)) {
            exprList->data = hd.e;
            return seq(hd.s, s);
        } else {
            Temp t = NewTemp();
            exprList->data = Ir_Tmp_Expr(t);
            return seq(hd.s, seq(Ir_Move_Stmt(Ir_Tmp_Expr(t), hd.e), s));
        }
    }
}

static IrStmt Do_Stmt(IrStmt stmt) {
    switch (stmt->kind) {
        case tIrSeq:
            return seq(Do_Stmt(stmt->as.seq->data),
                    Do_Stmt(stmt->as.seq->next->data));
        case tIrJump:
            return seq(Reorder(DataList(&stmt->as.jump.expr, NULL)), stmt);
        case tIrCjump:
            return seq(Reorder(DataList(&stmt->as.cjump.left,
                            DataList(&stmt->as.cjump.right, NULL))),
                        stmt);
        case tIrMove: {
            if (stmt->as.move.dst->kind == tIrTmp && stmt->as.move.src->kind == tIrCall) {
                return seq(Reorder(get_call_rlist(stmt->as.move.src)), stmt);
            } else if (stmt->as.move.dst->kind == tIrTmp) {
                return seq(Reorder(DataList(&stmt->as.move.src, NULL)), stmt);
            } else if (stmt->as.move.dst->kind == tIrMem) {
                return seq(Reorder(DataList(&stmt->as.move.dst->as.mem,
                                        DataList(&stmt->as.move.src, NULL))),
                            stmt);
            } else if (stmt->as.move.dst->kind == tIrEseq) {
                IrStmt s = stmt->as.move.dst->as.eseq.stmt;
                stmt->as.move.dst = stmt->as.move.dst->as.eseq.expr;
                return Do_Stmt(Ir_Seq_Stmt(DataList(s, DataList(stmt, NULL))));
            }
        }
        case tIrExpr:
                if (stmt->as.expr->kind == tIrCall) {
                    return seq(Reorder(get_call_rlist(stmt->as.expr)), stmt);
                } else {
                    return seq(Reorder(DataList(stmt->as.expr, NULL)), stmt);
                }
        default:
            return stmt;
    }
}


static struct C_StmtExpr Do_Expr(IrExpr expr) {
    switch(expr->kind) {
        case tIrBinop:
            return C_StmtExp(Reorder(DataList(&expr->as.binop.left,
                                DataList(&expr->as.binop.right, NULL))),
                    expr);
        case tIrMem:
            return C_StmtExp(Reorder(DataList(&expr->as.mem, NULL)), expr);
        case tIrEseq: {
            struct C_StmtExpr x = Do_Expr(expr->as.eseq.expr);
            return C_StmtExp(seq(Do_Stmt(expr->as.eseq.stmt), x.s), x.e);
        }
        case tIrCall:
            return C_StmtExp(Reorder(get_call_rlist(expr)), expr);
        default:
            return C_StmtExp(Reorder(NULL), expr);
    }
}

/* linear gets rid of the top-level SEQ's, producing a list */
static List/*<IrStmt>*/ Linear(IrStmt stmt, List/*<IrStmt>*/ listRight) {
    if (stmt->kind == tIrSeq) {
        return Linear(stmt->as.seq->data, Linear(stmt->as.seq->next->data, listRight));
    } else {
        return DataList(stmt, listRight);
    }
}

/* From an arbitrary Tree statement, produce a list of cleaned trees
   satisfying the following properties:
      1.  No SEQ's or ESEQ's
      2.  The parent of every CALL is an EXP(..) or a MOVE(TEMP t,..) */
List/*<IrStmt>*/ C_Linearize(IrStmt stmt) {
    return Linear(Do_Stmt(stmt), NULL);
}

static List/*<IrStmt>*/ mkBlocks(List/*<IrStmt>*/ stmts, TempLabel ldone);

/* Go down a list looking for end of basic block */
static List nextBlock(List prevstmts, List stmts, TempLabel ldone) {
    if (!stmts) {
        fprintf(stderr, "none left\n");
        return nextBlock(prevstmts, DataList(Ir_Jump_Stmt(Ir_Name_Expr(ldone), DataList(ldone, NULL)), NULL), ldone);
    }
    if (((IrStmt)stmts->data)->kind == tIrJump || ((IrStmt)stmts->data)->kind == tIrCjump) {
        fprintf(stderr, "Found ir jump\n");
        List stmLists;
        prevstmts->next = stmts;
        stmLists = mkBlocks(stmts->next, ldone);
        stmts->next = NULL;
        return stmLists;
    } else if (((IrStmt)stmts->data)->kind == tIrLabel) {
        fprintf(stderr, "Found irLabel\n");
        TempLabel lab = ((IrStmt)stmts->data)->as.label;
        return nextBlock(prevstmts, DataList(Ir_Jump_Stmt(Ir_Name_Expr(lab), DataList(lab, NULL)), stmts), ldone);
    } else {
        fprintf(stderr, "go down\n");
        /*assert(((IrStmt)stmts->data)->kind <= tIrExpr);*/
        prevstmts->next = stmts;
        return nextBlock(stmts, stmts->next, ldone);
    }
}

/* Create the beginning of a basic block */
static List/*<List<IrStmt>>*/ mkBlocks(List/*<IrStmt>*/ stmts, TempLabel ldone) {
    if (!stmts) {
        return NULL;
    }
    if (((IrStmt)stmts->data)->kind != tIrLabel) {
        return mkBlocks(DataList(Ir_Label_Stmt(NewLabel()), DataList(stmts, NULL)), ldone);
    }
    /* else there already is a label */
    return DataList(stmts, nextBlock(stmts, stmts->next, ldone));
}


struct C_Block C_BasicBlocks(List/*<IrStmt>*/ stmtList) {
    struct C_Block b;
    b.label = NewLabel();
    b.stmts = mkBlocks(stmtList, b.label);
    return b;
}

SymTable block_env = NULL;
struct C_Block global_block;

static List getLast(List list) {
    List last = list;
    while (last->next->next) {
        last = last->next;
    }
    return last;
}

static List getNext(void);

static void trace(List list) {
    List last = getLast(list);
    IrStmt lab = (IrStmt)list->data;
    IrStmt s = (IrStmt)last->next->data;
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
            ((List)last->next)->next = falses;
            trace(falses);
        } else if (trues) {	/* convert so that existing label is a false label */
            last->next->data = Ir_Cjump_Stmt(Ir_NotRel(s->as.cjump.op),
                    s->as.cjump.left, s->as.cjump.right, s->as.cjump.f, s->as.cjump.t);
            last->next->next = trues;
            trace(trues);
        } else {
            TempLabel falsel = NewLabel();
            last->next->data = Ir_Cjump_Stmt(s->as.cjump.op, s->as.cjump.left, s->as.cjump.right,
                s->as.cjump.t, falsel);
            last->next->next = DataList(Ir_Label_Stmt(falsel), getNext());
        }
    } else {
        assert(0);
    }
}

/* get the next block from the list of stmLists, using only those that have
 * not been traced yet */
static List getNext(void) {
    if (!global_block.stmts) {
        return DataList(Ir_Label_Stmt(global_block.label), NULL);
    } else {
        List s = (List)global_block.stmts->data;
        assert(((IrStmt)s->data)->kind == tIrLabel);
        if (SymTableLookup(block_env, ((IrStmt)s->data)->as.label)) {
            trace(s);
            return s;
        } else {
            global_block.stmts = global_block.stmts->next;
            return getNext();
        }
    }
}

List/*<IrStmt>*/ C_TraceSchedule(struct C_Block b) {
    List sList;
    block_env = MakeSymTable();
    global_block = b;

    for (sList = global_block.stmts; sList; sList = sList->next) {
        assert(((IrStmt)((List)sList->data)->data)->kind == tIrLabel);
        SymTableEnter(block_env, ((IrStmt)((List)sList->data)->data)->as.label, (List)sList->data);
    }

    return getNext();
}
