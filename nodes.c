#include "nodes.h"

Stmt CompoundStmt(Stmt stmt1, Stmt stmt2) {
    Stmt s = CHECKED_MALLOC(struct sStmt);
    s->kind = tCompoundStmt;
    s->as.compound.stmt1 = stmt1;
    s->as.compound.stmt2 = stmt2;
    return s;
}

Stmt AssignStmt(string id, Expr expr) {
    Stmt s = CHECKED_MALLOC(struct sStmt);
    s->kind = tAssignStmt;
    s->as.assign.id = id;
    s->as.assign.expr = expr;
    return s;
}

Stmt PrintStmt(ExprList exprs) {
    Stmt s = CHECKED_MALLOC(struct sStmt);
    s->kind = tPrintStmt;
    s->as.print.exprs = exprs;
    return s;
}

Expr IdExpr(string id) {
    Expr e = CHECKED_MALLOC(struct sExpr);
    e->kind = tIdExpr;
    e->as.id = id;
    return e;
}

Expr NumExpr(int num) {
    Expr e = CHECKED_MALLOC(struct sExpr);
    e->kind = tNumExpr;
    e->as.num = num;
    return e;
}

Expr OpExpr(Expr left, BinOp op, Expr right) {
    Expr e = CHECKED_MALLOC(struct sExpr);
    e->kind = tOpExpr;
    e->as.op.left = left;
    e->as.op.op = op;
    e->as.op.right = right;
    return e;
}

Expr SeqExpr(Stmt stmt, Expr expr) {
    Expr e = CHECKED_MALLOC(struct sExpr);
    e->kind = tSeqExpr;
    e->as.seq.stmt = stmt;
    e->as.seq.expr = expr;
    return e;
}

ExprList PairExprList(Expr head, ExprList tail) {
    ExprList e = CHECKED_MALLOC(struct sExprList);
    e->kind = tPairExprList;
    e->as.pair.head = head;
    e->as.pair.tail = tail;
    return e;
}

ExprList LastExprList(Expr last) {
    ExprList e = CHECKED_MALLOC(struct sExprList);
    e->kind = tLastExprList;
    e->as.last = last;
    return e;
}

