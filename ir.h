#ifndef tiger_ir_h
#define tiger_ir_h

#include "temp.h"
#include "util.h"

typedef struct sIrStmt *IrStmt;
typedef struct sIrExpr *IrExpr;

typedef enum {
    IR_PLUS, IR_MINUS, IR_MUL, IR_DIV,
    IR_AND, IR_OR, IR_XOR,
    IR_LSHIFT, IR_RSHIFT, IR_ARSHIFT,
} IrBinop;

typedef enum {
    IR_EQ, IR_NE, IR_LT, IR_LE, IR_GT, IR_GE,
    IR_ULT, IR_ULE, IR_UGT, IR_UGE,
} IrRelop;

struct sIrStmt {
    enum { tIrSeq, tIrLabel, tIrJump, tIrCjump, tIrMove, tIrExpr } kind;
    union {
        List/*<IrStmt>*/ seq;
        TempLabel label;
        struct { IrExpr expr; List jumps; } jump;
        struct { IrRelop op; IrExpr left, right; TempLabel t, f; } cjump;
        struct { IrExpr dst, src; } move;
        IrExpr expr;
    } as;
};

IrStmt Ir_Seq_Stmt(List/*<IrStmt>*/ seq);
IrStmt Ir_Label_Stmt(TempLabel label);
IrStmt Ir_Jump_Stmt(IrExpr expr, List jumps);
IrStmt Ir_Cjump_Stmt(IrRelop op,
                        IrExpr left,
                        IrExpr right,
                        TempLabel t,
                        TempLabel f);
IrStmt Ir_Move_Stmt(IrExpr dst, IrExpr src);
IrStmt Ir_Expr_Stmt(IrExpr expr);

struct sIrExpr {
    enum { tIrBinop, tIrMem, tIrTmp, tIrEseq, tIrName, tIrConst, tIrCall } kind;
    union {
        struct { IrBinop op; IrExpr left, right; } binop;
        IrExpr mem;
        Temp tmp;
        struct { IrStmt stmt; IrExpr expr; } eseq;
        TempLabel name;
        int const_;
        struct { IrExpr func; List args; } call;
    } as;
};

IrExpr Ir_Binop_Expr(IrBinop op, IrExpr left, IrExpr right);
IrExpr Ir_Mem_Expr(IrExpr mem);
IrExpr Ir_Tmp_Expr(Temp tmp);
// statement followed by expression
IrExpr Ir_Eseq_Expr(IrStmt stmt, IrExpr expr);
IrExpr Ir_Name_Expr(TempLabel name);
IrExpr Ir_Const_Expr(int const_);
IrExpr Ir_Call_Expr(IrExpr func, List args);

IrRelop Ir_NotRel(IrRelop op);

#endif
