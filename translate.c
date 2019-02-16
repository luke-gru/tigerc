#include "util.h"
#include "translate.h"

static TrLevel _outermost = NULL;

TrLevel Tr_Outermost(void) {
    if (!_outermost) {
        _outermost = Tr_NewLevel(NULL, NewLabel(), NULL);
    }
    return _outermost;
}

TrLevel Tr_NewLevel(TrLevel parent, TempLabel name, List formalEscapes) {
    TrLevel p = CHECKED_MALLOC(struct sTrLevel);
    p->parent = parent;
    p->frame = NewFrame(name, formalEscapes);
    List formalAccesses = NULL;
    List lastFormalAccess = NULL;
    while (formalEscapes) {
        if (formalAccesses) {
            lastFormalAccess->next = DataList(Tr_Access(p, formalAccesses->b), NULL);
            lastFormalAccess = lastFormalAccess->next;
        } else {
            TrAccess lastFormalAccess0 = Tr_Access(p, formalAccesses->b);
            formalAccesses = lastFormalAccess = DataList(lastFormalAccess0, NULL);
        }
        formalEscapes = formalEscapes->next;
    }
    p->formals = formalAccesses;
    p->locals = NULL;
    return p;
}

TrAccess Tr_Access(TrLevel level, bool escape) {
    TrAccess p = CHECKED_MALLOC(struct sTrAccess);
    p->level = level;
    p->escape = escape;
    return p;
}

List Tr_Formals(TrLevel level) {
    return level->formals;
}
