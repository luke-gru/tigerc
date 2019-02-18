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
