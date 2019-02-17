#ifndef tiger_translate_h
#define tiger_translate_h

#include "util.h"
#include "frame.h"
#include "temp.h"
#include "ir.h"

struct sTrCx {
    List trues;
    List falses;
    IrStmt stmt;
};

typedef struct sTrCx *TrCx;

struct sTrExpr {
    enum {
        tTrEx, // value expression
        tTrNx, // no value expression
        tTrCx // conditional expression
    } kind;
    union {
        IrExpr ex;
        IrStmt nx;
        TrCx cx;
    } as;
};

typedef struct sTrExpr *TrExpr;

struct sTrAccess; // fwd decls
struct sTrLevel;

typedef struct sTrAccess *TrAccess;
typedef struct sTrLevel  *TrLevel;

struct sTrAccess {
    TrLevel level;
    FAccess faccess;
};

struct sTrLevel {
    TrLevel parent;
    Frame frame;
    List/*<TrAccess>*/ formals; // formal access list
    List/*<TrAccess>*/ locals;  // local access list list
};

TrLevel Tr_Outermost(void);
TrLevel Tr_NewLevel(TrLevel parent, TempLabel name, List formalEscapes);
List Tr_Formals(TrLevel level);
TrAccess Tr_Access(TrLevel level, FAccess faccess);
TrAccess Tr_AllocLocal(TrLevel level, bool escape);

TrExpr Tr_NumExpr(int num);
TrExpr Tr_SimpleVar(TrAccess access, TrLevel level);
TrExpr Tr_AssignExpr(TrExpr lhs, TrExpr rhs);
TrExpr Tr_ArrayExpr(TrExpr size, TrExpr init);

TrExpr Tr_Ex(IrExpr irExpr);
TrExpr Tr_Nx(IrStmt irStmt);
TrExpr Tr_Cx(TrCx cx);
IrExpr Tr_UnEx(TrExpr trExpr);

#endif
