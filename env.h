#ifndef tiger_env_h
#define tiger_env_h

#include "util.h"
#include "types.h"
#include "symbol.h"
#include "temp.h"
#include "translate.h"

typedef struct sEnvEntry *EnvEntry;
struct sEnvEntry {
    enum {tVarEntry, tFunEntry} kind;
    union {
        struct {
            TrAccess access;
            Ty ty;
            bool isFor; // a "for" variable, to check we don't assign it
        } var;
        struct {
            TrLevel level;
            TempLabel label;
            List/*<Ty>*/formals;
            Ty result;
        } fun;
    } as;
};
EnvEntry E_VarEntry(TrAccess access, Ty ty, bool isFor);
EnvEntry E_FunEntry(TrLevel level, TempLabel label, List formals, Ty result);
SymTable E_Base_tEnv(void);/* Ty_ty environment */
SymTable E_Base_vEnv(void);/* EnvEntry environment */

#endif
