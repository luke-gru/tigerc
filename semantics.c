#include <stdio.h>
#include <stdarg.h>
#include "util.h"
#include "symbol.h"
#include "semantics.h"
#include "types.h"
#include "env.h"
#include "errormsg.h"
#include "translate.h"
#include "frame.h"
#include "temp.h"

#define PRINT_DECLS 0

#define CHECK_DEBUG(msg, ...) CheckDebug(msg, ##__VA_ARGS__)

static SymTable tEnv = NULL; // type sym table
static SymTable vEnv = NULL; // value sym table

static TrLevel curLevel;

#define CheckError(pos, msg, ...) EM_error(pos, msg, ##__VA_ARGS__)

/*static void CheckError(Pos pos, const char *msg, ...) {*/
    /*va_list ap;*/
    /*va_start(ap, msg);*/
    /*vfprintf(stderr, msg, ap);*/
    /*fprintf(stderr, "\n");*/
    /*va_end(ap);*/
/*}*/

static void CheckDebug(const char *msg, ...) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

static ExprTy VoidExprTy(void) {
    return ExprType(Tr_NumExpr(0), Ty_Void());
}

static ExprTy CheckExpr(N_Expr expr);
static ExprTy CheckVarExpr(N_Expr expr);
static ExprTy CheckVar(N_Var var);

static Ty LookupType(Symbol tyName, Pos pos) {
    Ty tyFound = SymTableLookup(tEnv, tyName);
    if (!tyFound) {
        CheckError(pos, "Undefined type '%s'", SymName(tyName));
        return NULL;
    } else {
        return Ty_Actual(tyFound);
    }
}

static ExprTy CheckVarDecl(N_Decl decl) {
    CHECK_DEBUG("checking varDecl");
    ExprTy initRes = CheckExpr(decl->as.var.init);
    Ty initTy = initRes.ty;
    TrAccess access = Tr_AllocLocal(curLevel, decl->as.var.escape);

    Symbol varName = decl->as.var.var;
    Symbol tyName = decl->as.var.ty;
    Ty tyFound = NULL;
    if (tyName) {
        tyFound = LookupType(tyName, decl->pos);
        if (!tyFound) {
            tyFound = Ty_Int();
        }
    } else {
        tyFound = initRes.ty;
    }

    if (!Ty_Match(tyFound, initTy)) {
        CheckError(decl->pos, "initializer has incorrect type");
    }
    SymTableEnter(vEnv, varName, E_VarEntry(access, tyFound, false));
    CHECK_DEBUG("/checking varDecl");
    assert(initRes.trExpr);
    ExprTy ret = ExprType(Tr_AssignExpr(Tr_SimpleVar(access, curLevel), initRes.trExpr), tyFound);
#if PRINT_DECLS
    fprintf(stdout, "Var decl '%s' IR:\n", SymName(varName));
    Tr_PPExpr(ret.trExpr);
#endif
    return ret;
}

static Ty CheckArrayType(N_Type ty) {
    Ty t = SymTableLookup(tEnv, ty->as.array);
    if (!t) {
        CheckError(ty->pos, "Undefined type '%s'", SymName(ty->as.array));
        t = Ty_Int();
    }
    return Ty_Array(t);
}

static Ty CheckNType(N_Type ty) {
    Ty t = SymTableLookup(tEnv, ty->as.name);
    if (!t) {
        CheckError(ty->pos, "undefined type '%s'", SymName(ty->as.name));
        t = Ty_Int();
    }
    return t;
}

static Ty CheckRecordType(N_Type ty) {
    List q = NULL, r = NULL;

    List p;
    for (p = ty->as.record; p; p = p->next) {
        N_Field field = p->data;
        Ty t = SymTableLookup(tEnv, field->ty);

        if (!t) {
            CheckError(ty->pos, "Undefined type '%s'", SymName(field->ty));
            t = Ty_Int();
        }

        if (r) {
            r->next = DataList(Ty_Field(field->name, t), NULL);
            r = r->next;
        } else {
            q = r = DataList(Ty_Field(field->name, t), NULL);
        }
    }

    return Ty_Record(q);
}

static Ty CheckType(N_Type ty) {
    switch (ty->kind) {
    case tNameTy:
        return CheckNType(ty);
    case tRecordTy:
        return CheckRecordType(ty);
    case tArrayTy:
        return CheckArrayType(ty);
    default:
        assert(0);
    }
}

static int CheckNameType(N_NameType nType) {
    CHECK_DEBUG("Entering type '%s'", SymName(nType->name));
    Ty tyFound = SymTableLookup(tEnv, nType->name);
    tyFound->as.name.ty = CheckType(nType->ty);
    return 0;
}

static int CheckTypesList(List typesList) {
    if (typesList == NULL) return 0;
    CheckNameType((N_NameType) typesList->data);
    return CheckTypesList(typesList->next);
}

static int CheckTypesDecl(N_Decl decl) {
    List typesList = decl->as.types;
    while (typesList) {
        N_NameType nType = (N_NameType)typesList->data;
        if (SymTableLookup(tEnv, nType->name) != NULL) {
            CheckError(nType->pos, "Type %s already declared");
        }
        // enter type placeholder for recursive types
        SymTableEnter(tEnv, nType->name, Ty_Name(nType->name, NULL));
        typesList = typesList->next;
    }
    int res =  CheckTypesList(decl->as.types);
    // check for infinite recursion in types
    typesList = decl->as.types;
    while (typesList) {
        N_NameType nType = (N_NameType)typesList->data;
        Ty tyFound = SymTableLookup(tEnv, nType->name);
        // tyFound should be a named type, if not it's an error
        if (tyFound == Ty_Actual(tyFound)) {
            CheckError(nType->pos, "infinite recursive type %s", SymName(nType->name));
        }
        typesList = typesList->next;
    }
    return res;
}

static List FormalTypeList(List params, Pos pos) {
    List q = NULL, r = NULL;
    while (params) {
        N_Field field = (N_Field)params->data;
        Ty t = LookupType(field->ty, pos);
        if (!t) {
            t = Ty_Int(); // error case, reported already
        }
        if (q) {
            r->next = DataList(t, NULL);
            r = r->next;
        } else {
            q = DataList(t, NULL);
            r = q;
        }
        params = params->next;
    }
    return q;
}

static List FormalEscapeList(List params) {
    List ret = NULL, last = NULL;
    while (params) {
        if (last) {
            last->next = BoolList(true, NULL); // TODO: analyze escapes
            last = last->next;
        } else {
            ret = last = BoolList(true, NULL);
        }
        params = params->next;
    }
    return ret;
}

static void CheckFunDecls(N_Decl decl) {
    List funcs = decl->as.functions;
    // enter stub function decl entries for mutually recursive function calls
    // in the declarations
    while (funcs) {
        N_FunDecl funcDecl = (N_FunDecl)funcs->data;
        List formals = FormalTypeList(funcDecl->params, funcDecl->pos);
        List escapes = FormalEscapeList(funcDecl->params);
        TempLabel funcLabel = NewLabel();
        Ty resultTy = Ty_Void();
        if (funcDecl->result) {
            resultTy = LookupType(funcDecl->result, funcDecl->pos);
        }
        TrLevel newLevel = Tr_NewLevel(curLevel, funcLabel, escapes);
        EnvEntry funcEntry = E_FunEntry(newLevel, funcLabel, formals, resultTy);
        SymTableEnter(vEnv, funcDecl->name, funcEntry);
        funcs = funcs->next;
    }

    funcs = decl->as.functions;
    while (funcs) {
        N_FunDecl funcDecl = (N_FunDecl)funcs->data;
        EnvEntry funcEntry = SymTableLookup(vEnv, funcDecl->name);
        SymTableBeginScope(vEnv);
        List params = funcDecl->params;
        List f = funcEntry->as.fun.formals;
        while (params) {
            N_Field nfield = (N_Field)params->data;
            Symbol fieldName = nfield->name;
            Ty formalTy = (Ty)f->data;
            TrAccess access = Tr_AllocLocal(curLevel, true);
            EnvEntry varEntry = E_VarEntry(access, formalTy, false);
            SymTableEnter(vEnv, fieldName, varEntry);
            params = params->next;
            f = f->next;
        }
        TrLevel oldLevel = curLevel;
        curLevel = funcEntry->as.fun.level;
        ExprTy bodyRes = CheckExpr(funcDecl->body);
#if PRINT_DECLS
        fprintf(stdout, "Function decl '%s' IR:\n", SymName(funcDecl->name));
        Tr_PPExpr(bodyRes.trExpr);
#endif

        Add_Frag(Proc_Frag(Tr_UnNx(bodyRes.trExpr), curLevel->frame, funcDecl->name));
        curLevel = oldLevel;
        SymTableEndScope(vEnv);
        funcs = funcs->next;
    }
}

static ExprTy CheckDecl(N_Decl decl) {
    switch (decl->kind) {
        case tFunctionDecl:
            CheckFunDecls(decl);
            return VoidExprTy();
        case tVarDecl:
            return CheckVarDecl(decl);
        case tTypesDecl:
            CheckTypesDecl(decl);
            return VoidExprTy();
        default:
            assert(0);
    }
}

// returns a list of declarations, in reverse order of found decls
static List/*<TrExpr>*/ CheckDecls(List decls) {
    List ret = NULL;
    while (decls) {
        N_Decl decl = (N_Decl)decls->data;
        ExprTy res = CheckDecl(decl);
        if (ret == NULL) {
            ret = DataList(res.trExpr, NULL);
        } else {
            ret = DataList(res.trExpr, ret);
        }
        decls = decls->next;
    }
    return ret;
}

// LET
//   decls
// IN
//   expr
static ExprTy CheckLetExpr(N_Expr expr) {
    SymTableBeginScope(tEnv);
    SymTableBeginScope(vEnv);
    List exprList = CheckDecls(expr->as.let.decls);
    assert(exprList);
    ExprTy res = CheckExpr(expr->as.let.body);
    exprList = DataList(res.trExpr, exprList); // prepend result
    SymTableEndScope(vEnv);
    SymTableEndScope(tEnv);
    return ExprType(Tr_SeqExpr(exprList), res.ty);
}

static ExprTy CheckSimpleVar(N_Var var) {
    EnvEntry varEntry = (EnvEntry)SymTableLookup(vEnv, var->as.simple);
    if (!varEntry) {
        CheckError(var->pos, "Variable '%s' not declared", SymName(var->as.simple));
        return ExprType(Tr_NumExpr(0), Ty_Int());
    } else if (varEntry->kind != tVarEntry) {
        CheckError(var->pos, "Expected '%s' to be a variable, not a function", SymName(var->as.simple));
        return ExprType(Tr_NumExpr(0), Ty_Int());
    }
    return ExprType(Tr_SimpleVar(varEntry->as.var.access, curLevel), Ty_Actual(varEntry->as.var.ty));
}

static ExprTy CheckFieldVar(N_Var var) {
    ExprTy recordVar = CheckVar(var->as.field.var);
    Ty recordTy = recordVar.ty;
    int i = 0;

    if (recordTy->kind != tTyRecord) {
        CheckError(var->pos, "Expected record type variable");
        return ExprType(Tr_NumExpr(0), Ty_Int());
    }

    List fields = recordTy->as.fields;
    while (fields) {
        TyField tfield = (TyField)fields->data;
        if (SymEq(tfield->name, var->as.field.sym)) {
            return ExprType(Tr_FieldVar(recordVar.trExpr, i), Ty_Actual(tfield->ty));
        }
        i++;
        fields = fields->next;
    }

    CheckError(var->pos, "There is no field named '%s'", SymName(var->as.field.sym));
    return ExprType(Tr_NumExpr(0), Ty_Int());
}

static ExprTy CheckSubscriptVar(N_Var svar) {
    N_Var arrayVar = svar->as.subscript.var;
    N_Expr exprVar = svar->as.subscript.expr;

    ExprTy arrayRes = CheckVar(arrayVar);
    if (arrayRes.ty->kind != tTyArray) {
        CheckError(svar->pos, "subscripted variable not an array");
    }
    ExprTy indexRes = CheckExpr(exprVar);
    if (indexRes.ty->kind != tTyInt) {
        CheckError(exprVar->pos, "array index needs to be of type int");
    }
    return ExprType(Tr_SubscriptVar(arrayRes.trExpr, indexRes.trExpr), arrayRes.ty->as.array);
}

static ExprTy CheckVar(N_Var var) {
    switch (var->kind) {
        case tSimpleVar:
            return CheckSimpleVar(var);
        case tFieldVar:
            return CheckFieldVar(var);
        case tSubscriptVar:
            return CheckSubscriptVar(var);
        default:
            assert(0);
    }
}

static ExprTy CheckVarExpr(N_Expr expr) {
    return CheckVar(expr->as.var);
}

static ExprTy CheckSeqExpr(N_Expr expr) {
    List exprList = expr->as.seq;
    ExprTy ret = VoidExprTy();
    List retList = NULL;
    while (exprList) {
        ret = CheckExpr((N_Expr)exprList->data);
        if (retList) {
            retList = DataList(ret.trExpr, retList); // prepend
        } else {
            retList = DataList(ret.trExpr, NULL);
        }
        exprList = exprList->next;
    }
    if (!retList) {
        retList = DataList(Tr_NoExp(), NULL);
    }
    return ExprType(Tr_SeqExpr(retList), Ty_Void());
}

static ExprTy CheckAssignExpr(N_Expr expr) {
    N_Var var = expr->as.assign.var;
    N_Expr aexpr = expr->as.assign.expr;
    ExprTy varRes = CheckVar(var);
    ExprTy exprRes = CheckExpr(aexpr);

    if (!Ty_Match(varRes.ty, exprRes.ty)) {
        CheckError(expr->pos, "Type mismatch");
    }
    return ExprType(Tr_AssignExpr(varRes.trExpr, exprRes.trExpr), Ty_Void());
}

static ExprTy CheckIfExpr(N_Expr expr) {
    ExprTy testRes = CheckExpr(expr->as.iff.test);
    if (testRes.ty->kind != tTyInt) {
        CheckError(expr->pos, "if test condition must evaluate to int");
    }
    ExprTy ifRes = CheckExpr(expr->as.iff.then);
    /*if (ifRes.ty->kind != tTyVoid) {*/
        /*CheckError(expr->as.iff.then->pos, "if 'then' condition must evaluate to unit type");*/
    /*}*/
    if (expr->as.iff.elsee) {
        ExprTy elseRes = CheckExpr(expr->as.iff.elsee);
        if (elseRes.ty->kind != ifRes.ty->kind) {
            CheckError(expr->pos, "if 'else' condition must evaluate to same type as 'if' condition");
        }
        return ExprType(Tr_IfExpr(testRes.trExpr, ifRes.trExpr, elseRes.trExpr), Ty_Void());
    } else {
        return ExprType(Tr_IfExpr(testRes.trExpr, ifRes.trExpr, NULL), Ty_Void());
    }
}

static ExprTy CheckWhileExpr(N_Expr expr) {
    ExprTy testRes = CheckExpr(expr->as.whilee.test);
    if (testRes.ty->kind != tTyInt) {
        CheckError(expr->pos, "while test condition must evaluate to int");
    }
    ExprTy bodyRes = CheckExpr(expr->as.whilee.body);
    if (bodyRes.ty->kind != tTyVoid) {
        CheckError(expr->pos, "While body must evaluate to unit type");
    }
    return ExprType(Tr_WhileExpr(testRes.trExpr, bodyRes.trExpr), bodyRes.ty);
}

static ExprTy CheckForExpr(N_Expr expr) {
    ExprTy loRes = CheckExpr(expr->as.forr.lo);
    if (loRes.ty->kind != tTyInt) {
        CheckError(expr->as.forr.lo->pos, "for loop low expr must evaluate to int");
    }
    ExprTy hiRes = CheckExpr(expr->as.forr.hi);
    if (hiRes.ty->kind != tTyInt) {
        CheckError(expr->as.forr.hi->pos, "for loop high expr must evaluate to int");
    }
    SymTableBeginScope(vEnv);
    TrAccess varAccess = Tr_AllocLocal(curLevel, true);
    SymTableEnter(vEnv, expr->as.forr.var, E_VarEntry(varAccess, Ty_Int(), true));
    ExprTy bodyRes = CheckExpr(expr->as.forr.body);
    if (bodyRes.ty->kind != tTyVoid) {
        CheckError(expr->as.forr.body->pos, "for body should evaluate to unit type");
    }
    SymTableEndScope(vEnv);
    return ExprType(Tr_ForExpr(varAccess, loRes.trExpr, hiRes.trExpr, bodyRes.trExpr), Ty_Void());
}

static ExprTy CheckNilExpr(N_Expr expr) {
    return ExprType(Tr_NumExpr(0), Ty_Nil());
}

static ExprTy CheckIntExpr(N_Expr expr) {
    return ExprType(Tr_NumExpr(expr->as.intVal), Ty_Int());
}

static ExprTy CheckStringExpr(N_Expr expr) {
    return ExprType(Tr_StringExpr(expr->as.stringVal), Ty_String());
}

static ExprTy CheckCallExpr(N_Expr expr) {
    Symbol funcName = expr->as.call.func;
    List argExprs = expr->as.call.args;
    EnvEntry funcEntry = SymTableLookup(vEnv, funcName);
    int nArg = 0;
    List argExprList = NULL, argExprNext = NULL;
    if (!funcEntry) {
        CheckError(expr->pos, "Function '%s' not declared", SymName(funcName));
        return VoidExprTy();
    }
    if (funcEntry->kind == tVarEntry) {
        CheckError(expr->pos, "Expected function, found variable ('%s')", SymName(funcName));
        return VoidExprTy();
    }
    List formalTys = funcEntry->as.fun.formals;
    Ty formalResTy = funcEntry->as.fun.result;

    while (argExprs && formalTys) {
        nArg++;
        N_Expr argExpr = (N_Expr)argExprs->data;
        ExprTy argRes = CheckExpr(argExpr);
        Ty formalTy = formalTys->data;
        if (!Ty_Match(formalTy, argRes.ty)) {
            CheckError(argExpr->pos, "Argument type mismatch for argument %d in function '%s'",
                    nArg, SymName(funcName));
        }

        if (argExprList) {
            argExprNext->next = DataList(argRes.trExpr, NULL);
            argExprNext = argExprNext->next;
        } else {
            argExprList = DataList(argRes.trExpr, NULL);
            argExprNext = argExprList;
        }

        argExprs = argExprs->next;
        formalTys = formalTys->next;
    }

    if (formalTys) {
        CheckError(expr->pos, "Expect more arguments");
    } else if (argExprs) {
        CheckError(expr->pos, "Expect less arguments");
    }

    return ExprType(Tr_CallExpr(funcEntry->as.fun.level, funcEntry->as.fun.label, argExprList), formalResTy);
}

static ExprTy CheckOpExpr(N_Expr expr) {
    Op op = expr->as.op.op;
    ExprTy left = CheckExpr(expr->as.op.left);
    ExprTy right = CheckExpr(expr->as.op.right); // FIXME: uminus?

    switch (op) {
        case PlusOp:
        case MinusOp:
        case TimesOp:
        case DivideOp:
            if (left.ty->kind != tTyInt) {
                CheckError(expr->as.op.left->pos, "int required");
            }
            if (right.ty->kind != tTyInt) {
                CheckError(expr->as.op.right->pos, "int required");
            }
            return ExprType(Tr_BinopExpr(op-PlusOp+IR_PLUS, left.trExpr, right.trExpr), Ty_Int());
        case EqOp:
        case NeqOp:
            if (!Ty_Match(left.ty, right.ty)) {
                CheckError(expr->pos, "the type of two operands must be the same");
            }
            return ExprType(Tr_RelopExpr(op-EqOp+IR_EQ, left.trExpr, right.trExpr), Ty_Int());
        case LtOp:
        case LeOp:
        case GtOp:
        case GeOp:
            if (!Ty_Match(left.ty, right.ty)) {
                CheckError(expr->pos, "the type of two operands must be the same");
            }

            if (left.ty->kind != tTyInt && left.ty->kind != tTyString) {
                CheckError(expr->pos, "the type of comparison's operand must be int or string");
            }
            return ExprType(Tr_RelopExpr(op-LtOp+IR_LT, left.trExpr, right.trExpr), left.ty);
    }
    assert(0);
}

static ExprTy CheckArrayExpr(N_Expr expr) {
    Ty arrayTy = LookupType(expr->as.array.ty, expr->pos);
    ExprTy sizeRes = CheckExpr(expr->as.array.size);
    ExprTy initRes = CheckExpr(expr->as.array.init);

    if (!arrayTy) {
        return VoidExprTy();
    }
    if (arrayTy->kind != tTyArray) {
        CheckError(expr->pos, "'%s' is not an array type", SymName(expr->as.array.ty));
    }

    if (sizeRes.ty->kind != tTyInt) {
        CheckError(expr->pos, "array size must be of type int");
    }

    if (!Ty_Match(arrayTy->as.array, initRes.ty)) {
        CheckError(expr->pos, "array initializer has incorrect type");
    }
    return ExprType(Tr_ArrayExpr(sizeRes.trExpr, initRes.trExpr), arrayTy);
}

static ExprTy CheckRecordExpr(N_Expr expr) {
    Ty recTy = LookupType(expr->as.record.ty, expr->pos);
    List p, q;
    List fieldsList = NULL, next = NULL;
    int size = 0;

    if (!recTy) {
        return ExprType(Tr_NumExpr(0), Ty_Nil());
    }
    if (recTy->kind != tTyRecord) {
        CheckError(expr->pos, "'%s' is not a record type",
            SymName(expr->as.record.ty));
    }

    for (p = recTy->as.fields, q = expr->as.record.efields;
        p && q;
        p = p->next, q = q->next, size++) {

        N_EField efield = (N_EField)q->data;
        ExprTy eFieldRes = CheckExpr(efield->expr);
        TyField tyField = (TyField)p->data;
        if (!Ty_Match(tyField->ty, eFieldRes.ty)) {
            CheckError(efield->pos, "wrong field type");
        }
        if (fieldsList) {
            next->next = DataList(eFieldRes.trExpr, NULL);
            next = next->next;
        } else {
            fieldsList = next = DataList(eFieldRes.trExpr, NULL);
        }
    }

    if (p || q) {
        CheckError(expr->pos, "wrong field number");
    }
    return ExprType(Tr_RecordExpr(fieldsList, size), recTy);
}

static ExprTy CheckExpr(N_Expr expr) {
    switch (expr->kind) {
        case tVarExpr:
            return CheckVarExpr(expr);
        case tNilExpr:
            return CheckNilExpr(expr);
        case tIntExpr:
            return CheckIntExpr(expr);
        case tStringExpr:
            return CheckStringExpr(expr);
        case tCallExpr:
            return CheckCallExpr(expr);
        case tOpExpr:
            return CheckOpExpr(expr);
        case tRecordExpr:
            return CheckRecordExpr(expr);
        case tLetExpr:
            return CheckLetExpr(expr);
        case tSeqExpr:
            return CheckSeqExpr(expr);
        case tAssignExpr:
            return CheckAssignExpr(expr);
        case tIfExpr:
            return CheckIfExpr(expr);
        case tWhileExpr:
            return CheckWhileExpr(expr);
        case tForExpr:
            return CheckForExpr(expr);
        case tArrayExpr:
            return CheckArrayExpr(expr);
        default:
            fprintf(stderr, "Unhandled type check for expr kind: %d\n", expr->kind);
            assert(0);
    }
}

struct sExprTy ExprType(TrExpr trExpr, Ty ty) {
    struct sExprTy e;
    e.trExpr = trExpr;
    e.ty = ty;
    return e;
}

List/*<Frag>*/ TypeCheck(N_Expr program) {
    tEnv = E_Base_tEnv();
    vEnv = E_Base_vEnv();
    curLevel = Tr_Outermost();
    ExprTy res = CheckExpr(program);
    TempLabel mainLbl = NamedLabel("_main");
    Add_Frag(Proc_Frag(Tr_UnNx(res.trExpr), curLevel->frame, mainLbl));
    return Tr_getResult();
}
