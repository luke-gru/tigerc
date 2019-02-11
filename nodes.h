#ifndef tiger_nodes_h
#define tiger_nodes_h

#include "util.h"

struct sStmt;
struct sExpr;
struct sExprList;

typedef struct sStmt *Stmt;
typedef struct sExpr *Expr;
typedef struct sExprList *ExprList;
typedef enum { OpPlus,OpMinus,OpTimes,OpDiv } BinOp;

struct sStmt {
    enum {
        tCompoundStmt,
        tAssignStmt,
        tPrintStmt,
    } kind;
    union {
        struct {
            Stmt stmt1;
            Stmt stmt2;
        } compound;
        struct {
            string id;
            Expr expr;
        } assign;
        struct {
            ExprList exprs;
        } print;
    } as;
};

struct sExpr {
    enum {
        tIdExpr, tNumExpr, tOpExpr, tSeqExpr,
    } kind;
    union {
        string id;
        int num;
        struct {
            Expr left;
            BinOp op;
            Expr right;
        } op;
        struct {
            Stmt stmt;
            Expr expr;
        } seq;
    } as;
};

struct sExprList {
    enum {
        tPairExprList, tLastExprList,
    } kind;
    union {
        struct {
            Expr head;
            ExprList tail;
        } pair;
        Expr last;
    } as;
};

#define FOREACH_EXPRLIST(exprList, expr, exec) \
    ExprList cur = exprList;\
    while (cur) {\
        switch (cur->kind) {\
            case tPairExprList:\
                expr = cur->as.pair.head;\
                cur = cur->as.pair.tail;\
                break;\
            case tLastExprList:\
                expr = cur->as.last;\
                cur = NULL;\
                break;\
        }\
        exec\
    }\

Stmt CompoundStmt(Stmt stmt1, Stmt stmt2);
Stmt AssignStmt(string id, Expr expr);
Stmt PrintStmt(ExprList exprs);

Expr IdExpr(string id);
Expr NumExpr(int num);
Expr OpExpr(Expr left, BinOp op, Expr right);
Expr SeqExpr(Stmt stmt, Expr expr);

ExprList PairExprList(Expr head, ExprList tail);
ExprList LastExprList(Expr last);

#endif
