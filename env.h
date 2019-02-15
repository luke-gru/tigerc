#ifndef tiger_env_h
#define tiger_env_h

#include "util.h"
#include "types.h"
#include "symbol.h"

typedef struct sEnvEntry *EnvEntry;
struct sEnvEntry {
    enum {tVarEntry, tFunEntry} kind;
    union {
        struct {Ty ty;} var;
        struct {
            List/*<Ty>*/formals;
            Ty result;
        } fun;
    } as;
};
EnvEntry E_VarEntry(Ty ty);
EnvEntry E_FunEntry(List formals, Ty result);
SymTable E_Base_tEnv(void);/* Ty_ty environment */
SymTable E_Base_vEnv(void);/* EnvEntry environment */

#endif
