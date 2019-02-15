#include "env.h"

EnvEntry E_VarEntry(Ty ty) {
    EnvEntry p = CHECKED_MALLOC(struct sEnvEntry);
    p->kind = tVarEntry;
    p->as.var.ty = ty;
    return p;
}

EnvEntry E_FunEntry(List formals, Ty result) {
    EnvEntry p = CHECKED_MALLOC(struct sEnvEntry);
    p->kind = tFunEntry;
    p->as.fun.formals = formals;
    p->as.fun.result = result;
    return p;
}

SymTable E_Base_tEnv(void) {
    SymTable tab = MakeSymTable();
    SymTableEnter(tab, GetSym("int"), Ty_Int());
    SymTableEnter(tab, GetSym("string"), Ty_String());
    return tab;
}

SymTable E_Base_vEnv(void) {
    SymTable tab = MakeSymTable();
    return tab;
}
