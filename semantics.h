#ifndef tiger_semantics_h
#define tiger_semantics_h

#include "ast.h"
#include "translate.h" // for Tr_Expr
#include "types.h"

struct sExprTy { TrExpr trExpr; Ty ty; };

typedef struct sExprTy ExprTy;

struct sExprTy ExprType(TrExpr trExpr, Ty ty);

List/*<Frag>*/ TypeCheck(N_Expr program);

#endif
