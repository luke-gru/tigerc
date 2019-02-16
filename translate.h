#ifndef tiger_translate_h
#define tiger_translate_h

#include "util.h"
#include "frame.h"
#include "temp.h"

typedef void *Tr_Expr; // todo

struct sTrAccess; // fwd decls
struct sTrLevel;

typedef struct sTrAccess *TrAccess;
typedef struct sTrLevel  *TrLevel;

struct sTrAccess {
    TrLevel level;
    bool escape;
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
TrAccess Tr_Access(TrLevel level, bool escape);

#endif
