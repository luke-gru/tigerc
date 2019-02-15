#include <stdio.h>
#include <stdarg.h>
#include "util.h"
#include "symbol.h"
#include "semantics.h"
#include "types.h"
#include "env.h"

#define CHECK_DEBUG(msg, ...) CheckDebug(msg, ##__VA_ARGS__)

static SymTable tEnv = NULL; // type sym table
static SymTable vEnv = NULL; // value sym table

static void CheckError(Pos pos, const char *msg, ...) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

static void MismatchTypeError(Pos pos, Ty expected, Ty got) {
    assert(!Ty_Match(expected, got));
    string expectedName = Ty_GetName(expected);
    string gotName = Ty_GetName(got);
    CheckError(pos, "Expected type %s, got %s", expectedName, gotName);
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

static int CheckExprType(N_Expr expr, Ty ty, Symbol tyName) {
    Ty tyFound;
    string tyFoundName;
    switch (expr->kind) {
        case tVarExpr:
        case tCallExpr:
        case tRecordExpr:
        case tSeqExpr:
            return 0; // TODO
        case tNilExpr:
            tyFound = Ty_Nil();
            tyFoundName = "nil";
            break;
        case tIntExpr:
        case tOpExpr:
            tyFound = Ty_Int();
            tyFoundName = "nil";
            break;
        case tStringExpr:
            tyFound = Ty_String();
            tyFoundName = "string";
            break;
        case tArrayExpr: {
            Symbol arrayTySym = expr->as.array.ty;
            Ty arrayTy = (Ty)SymTableLookup(tEnv, arrayTySym);
            if (!arrayTy) {
                CheckError(expr->pos, "Type '%s' not found", SymName(arrayTySym));
                return -1;
            }
            if (arrayTy->kind != tTyArray) {
                CheckError(expr->pos, "Expected array type, got type %s", Ty_GetName(arrayTy));
                return -1;
            }
            N_Expr arraySzExpr = expr->as.array.size;

            // check size
            assert(arraySzExpr);
            int szRes = CheckExprType(arraySzExpr, Ty_Int(), GetSym("int"));
            if (szRes != 0) return szRes;

            // check init
            N_Expr arrayInitExpr = expr->as.array.init;
            assert(arrayInitExpr);
            int initRes = CheckExprType(arrayInitExpr, arrayTy->as.array, GetSym("int"));
            if (initRes != 0) return initRes;

            tyFound = arrayTy;
            tyFoundName = SymName(arrayTySym);
            break;
        }
        default:
            fprintf(stderr, "found expr of kind: %d\n", expr->kind);
            assert(0);
    }
    if (!Ty_Match(tyFound, ty)) {
        CheckError(expr->pos, "Type '%s' found, expected type '%s'", tyFoundName, SymName(tyName));
        return -1;
    }
    return 0;
}

// TODO: check usage of types matches initializer, if given
static int CheckVarDecl(N_Decl decl) {
    CHECK_DEBUG("checking varDecl");
    Symbol varName = decl->as.var.var;
    Symbol tyName = decl->as.var.ty;
    if (SymTableLookup(tEnv, varName) != NULL) {
        CheckError(decl->pos, "Variable %s already declared", SymName(varName));
        return -1;
    }
    Ty tyFound = (Ty)SymTableLookup(tEnv, tyName);
    if (!tyFound) {
        CheckError(decl->pos, "Type '%s' not found", SymName(tyName));
        return -1;
    }
    SymTableEnter(tEnv, varName, tyFound);
    if (decl->as.var.init) {
        int res = CheckExprType(decl->as.var.init, tyFound, tyName);
        if (res != 0) return res;
    }
    return 0;
}

static int CheckNameType(N_NameType nType) {
    if (SymTableLookup(tEnv, nType->name) != NULL) {
        CheckError(nType->pos, "Type %s already declared");
        return -1;
    }
    CHECK_DEBUG("Entering type '%s'", SymName(nType->name));
    SymTableEnter(tEnv, nType->name, nType->ty);
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
    SymTableBeginScope(vEnv);
    int declRes = CheckDecls(expr->as.let.decls);
    int res = CheckExpr(expr->as.let.body);
    SymTableEndScope(vEnv);
    return res == 0 && declRes == 0;
}

static int CheckSimpleVar(N_Var var) {
    if (!SymTableLookup(vEnv, var->as.simple)) {
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

static int CheckSeqExprList(List exprList) {
    if (exprList == NULL) return 0;
    int res = CheckExpr((N_Expr)exprList->data);
    if (res != 0) return res;
    return CheckSeqExprList(exprList->next);
}

static int CheckSeqExpr(N_Expr expr) {
    return CheckSeqExprList(expr->as.seq);
}

static Ty FindFieldType(Ty recordType, Symbol fieldName) {
    assert(recordType->kind == tTyRecord);
    List fields = recordType->as.fields;
    while (fields) {
        TyField f = (TyField)fields->data;
        if (SymEq(f->name, fieldName)) {
            return f->ty;
        }
        fields = fields->next;
    }
    return NULL;
}

static Ty FindVarType(N_Var var) {
    switch (var->kind) {
        case tSimpleVar: {
            Symbol varName = var->as.simple;
            return SymTableLookup(tEnv, varName);
        }
        case tFieldVar: {
            Ty recordTy = FindVarType(var->as.field.var);
            if (!recordTy) return NULL;
            assert(recordTy->kind == tTyRecord);
            Ty fieldType = FindFieldType(recordTy, var->as.field.sym);
            if (!fieldType) {
                CheckError(var->pos, "Record doesn't have field '%s'", SymName(var->as.field.sym));
                return NULL;
            }
            return fieldType;
        }
        case tSubscriptVar:
            assert(0);
    }
    return NULL;
}

static Ty FindOpExprType(N_Expr expr) {
    switch (expr->as.op.op) {
        case PlusOp:
        case MinusOp:
        case TimesOp:
        case DivideOp:
            return Ty_Int();
        case EqOp:
        case NeqOp:
        case LtOp:
        case LeOp:
        case GtOp:
        case GeOp:
            return Ty_Int(); // int as bool
        default:
            assert(0);
    }
}

static Ty FindExprType(N_Expr expr) {
    switch (expr->kind) {
        case tStringExpr:
            return Ty_String();
        case tIntExpr:
            return Ty_Int();
        case tNilExpr:
            return Ty_Nil();
        case tOpExpr:
            return FindOpExprType(expr);
        default:
            assert(0);
    }
    return NULL;
}

static int CheckFieldVarMatches(N_Var var, N_Expr expr) {
    Ty varType = FindVarType(var);
    if (!varType) return -1;
    Ty exprType = FindExprType(expr);
    if (!Ty_Match(varType, exprType)) {
        MismatchTypeError(var->pos, varType, exprType);
        return -1;
    }
    return 0;
}

static int CheckAssignExpr(N_Expr expr) {
    N_Var var = expr->as.assign.var;
    switch (var->kind) {
        case tSimpleVar:
            break;
        case tFieldVar:
            return CheckFieldVarMatches(var, expr->as.assign.expr);
        case tSubscriptVar:
            break;
        default:
            assert(0);
    }
    return 0;
}

static int CheckExpr(N_Expr expr) {
    switch (expr->kind) {
        case tLetExpr:
            return CheckLetExpr(expr);
        case tVarExpr:
            return CheckVarExpr(expr);
        case tSeqExpr:
            return CheckSeqExpr(expr);
        case tAssignExpr:
            return CheckAssignExpr(expr);
        default:
            fprintf(stderr, "Unhandled type check for expr kind: %d\n", expr->kind);
            assert(0);
            return -1;
    }
}

int TypeCheck(N_Expr program) {
    tEnv = E_Base_tEnv();
    vEnv = E_Base_vEnv();
    return CheckExpr(program);
}
