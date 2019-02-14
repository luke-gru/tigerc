#include <stdio.h>
#include <stdarg.h>
#include "util.h"
#include "symbol.h"
#include "semantics.h"

#define CHECK_DEBUG(msg, ...) CheckDebug(msg, ##__VA_ARGS__)

static SymTable symTab = NULL;

static void CheckError(Pos pos, const char *msg, ...) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

static void CheckDebug(const char *msg, ...) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

static int CheckExpr(N_Expr expr);

static int CheckFunDecl(N_Decl decl) {
    return 0;
}

// TODO: check usage of types matches initializer, if given
static int CheckVarDecl(N_Decl decl) {
    CHECK_DEBUG("checking varDecl");
    Symbol varName = decl->as.var.var;
    Symbol tyName = decl->as.var.ty;
    if (SymTableLookup(symTab, varName) != NULL) {
        CheckError(decl->pos, "Variable %s already declared", SymName(varName));
        return -1;
    }
    N_Type tyFound = (N_Type)SymTableLookup(symTab, tyName);
    if (!tyFound) {
        CheckError(decl->pos, "Type '%s' not found", SymName(tyName));
        return -1;
    }
    SymTableEnter(symTab, varName, tyFound);
    return 0;
}

static int CheckNameType(N_NameType nType) {
    if (SymTableLookup(symTab, nType->name) != NULL) {
        CheckError(nType->pos, "Type %s already declared");
        return -1;
    }
    CHECK_DEBUG("Entering type '%s'", SymName(nType->name));
    SymTableEnter(symTab, nType->name, nType->ty);
    return 0;
}

static int CheckTypesList(List typesList) {
    if (typesList == NULL) return 0;
    CheckNameType((N_NameType) typesList->data);
    return CheckTypesList(typesList->next);
}

static int CheckTypesDecl(N_Decl decl) {
    List typesList = decl->as.types;
    return CheckTypesList(typesList);
}

static int CheckDecl(N_Decl decl) {
    switch (decl->kind) {
        case tFunctionDecl:
            return CheckFunDecl(decl);
        case tVarDecl:
            return CheckVarDecl(decl);
        case tTypesDecl:
            return CheckTypesDecl(decl);
        default:
            assert(0);
    }
}

static int CheckDecls(List decls) {
    if (decls == NULL) return 0;
    N_Decl decl = (N_Decl)decls->data;
    int res = CheckDecl(decl);
    if (res != 0) return res;
    return CheckDecls(decls->next);
}

// LET
//   decls
// IN
//   expr
static int CheckLetExpr(N_Expr expr) {
    SymTableBeginScope(symTab);
    CheckDecls(expr->as.let.decls);
    int res = CheckExpr(expr->as.let.body);
    SymTableEndScope(symTab);
    return res;
}

static int CheckSimpleVar(N_Var var) {
    if (!SymTableLookup(symTab, var->as.simple)) {
        CheckError(var->pos, "Variable '%s' not declared", SymName(var->as.simple));
        return -1;
    }
    return 0;
}

static int CheckFieldVar(N_Var var) {
    return 0;
}

static int CheckSubscriptVar(N_Var var) {
    return 0;
}

static int CheckVarExpr(N_Expr expr) {
    switch (expr->as.var->kind) {
        case tSimpleVar:
            return CheckSimpleVar(expr->as.var);
        case tFieldVar:
            return CheckFieldVar(expr->as.var);
        case tSubscriptVar:
            return CheckSubscriptVar(expr->as.var);
        default:
            assert(0);
    }
}

static int CheckExpr(N_Expr expr) {
    switch (expr->kind) {
        case tLetExpr:
            return CheckLetExpr(expr);
        case tVarExpr:
            return CheckVarExpr(expr);
        default:
            fprintf(stderr, "Unhandled type check for expr kind: %d\n", expr->kind);
            assert(0);
            return -1;
    }
}

int TypeCheck(N_Expr program) {
    symTab = MakeSymTable();
    return CheckExpr(program);
}
