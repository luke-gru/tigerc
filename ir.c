#include "util.h"
#include "ir.h"

static IrStmt AllocIrStmt(int kind) {
    IrStmt p = CHECKED_MALLOC(struct sIrStmt);
    p->kind = kind;
    return p;
}

static IrExpr AllocIrExpr(int kind) {
    IrExpr p = CHECKED_MALLOC(struct sIrExpr);
    p->kind = kind;
    return p;
}

IrStmt Ir_Seq_Stmt(List seq) {
    IrStmt stmt = AllocIrStmt(tIrSeq);
    stmt->as.seq = seq;
    return stmt;
}

IrStmt Ir_Label_Stmt(TempLabel label) {
    IrStmt stmt = AllocIrStmt(tIrLabel);
    stmt->as.label = label;
    return stmt;
}

IrStmt Ir_Jump_Stmt(IrExpr expr, List jumps) {
    IrStmt stmt = AllocIrStmt(tIrJump);
    stmt->as.jump.expr = expr;
    stmt->as.jump.jumps = jumps;
    return stmt;
}

IrStmt Ir_Cjump_Stmt(IrRelop op,
                        IrExpr left,
                        IrExpr right,
                        TempLabel t,
                        TempLabel f) {
    IrStmt stmt = AllocIrStmt(tIrCjump);
    stmt->as.cjump.op = op;
    stmt->as.cjump.left = left;
    stmt->as.cjump.right = right;
    stmt->as.cjump.t = t;
    stmt->as.cjump.f = f;
    return stmt;
}

IrStmt Ir_Move_Stmt(IrExpr dst, IrExpr src) {
    IrStmt stmt = AllocIrStmt(tIrMove);
    stmt->as.move.dst = dst;
    stmt->as.move.src = src;
    return stmt;
}

IrStmt Ir_Expr_Stmt(IrExpr expr) {
    IrStmt stmt = AllocIrStmt(tIrExpr);
    stmt->as.expr = expr;
    return stmt;
}

IrExpr Ir_Binop_Expr(IrBinop op, IrExpr left, IrExpr right) {
    IrExpr p = AllocIrExpr(tIrBinop);
    p->as.binop.op = op;
    p->as.binop.left = left;
    p->as.binop.right = right;
    return p;
}

IrExpr Ir_Mem_Expr(IrExpr mem) {
    IrExpr p = AllocIrExpr(tIrMem);
    p->as.mem = mem;
    return p;
}

IrExpr Ir_Tmp_Expr(Temp tmp) {
    IrExpr p = AllocIrExpr(tIrTmp);
    p->as.tmp = tmp;
    return p;
}

IrExpr Ir_Eseq_Expr(IrStmt stmt, IrExpr expr) {
    IrExpr p = AllocIrExpr(tIrEseq);
    p->as.eseq.stmt = stmt;
    p->as.eseq.expr = expr;
    return p;
}

IrExpr Ir_Name_Expr(TempLabel name) {
    IrExpr p = AllocIrExpr(tIrName);
    p->as.name = name;
    return p;
}

IrExpr Ir_Const_Expr(int const_) {
    IrExpr p = AllocIrExpr(tIrConst);
    p->as.const_ = const_;
    return p;
}

IrExpr Ir_Call_Expr(IrExpr func, List args) {
    IrExpr p = AllocIrExpr(tIrCall);
    p->as.call.func = func;
    p->as.call.args = args;
    return p;
}

IrRelop Ir_NotRel(IrRelop op) {
    switch (op) {
    case IR_EQ:
        return IR_NE;
    case IR_NE:
        return IR_EQ;
    case IR_LT:
        return IR_GE;
    case IR_LE:
        return IR_GT;
    case IR_GT:
        return IR_LE;
    case IR_GE:
        return IR_LT;
    case IR_ULT:
        return IR_UGE;
    case IR_ULE:
        return IR_UGT;
    case IR_UGT:
        return IR_ULE;
    case IR_UGE:
        return IR_ULT;
    default:
        assert(0);
    }
}

