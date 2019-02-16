#ifndef tiger_semantics_h
#define tiger_semantics_h

#include "ast.h"
#include "translate.h" // for Tr_Expr
#include "types.h"

struct sExprTy { Tr_Expr trExpr; Ty ty; };

typedef struct sExprTy ExprTy;

struct sExprTy ExprType(Tr_Expr trExpr, Ty ty);

ExprTy TypeCheck(N_Expr program);

#endif
