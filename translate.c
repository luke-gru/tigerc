#include <stdio.h>
#include "util.h"
#include "translate.h"
#include "frame.h"

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
    // extra stack parameter for static link to parent frame
    p->frame = NewFrame(name, BoolList(true, formalEscapes));
    List formalFaccesses = FrameFormals(p->frame);
    List formalAccesses = NULL;
    List lastFormalAccess = NULL;
    while (formalFaccesses) {
        if (formalAccesses) {
            lastFormalAccess->next = DataList(Tr_Access(p, (FAccess)formalFaccesses->data), NULL);
            lastFormalAccess = lastFormalAccess->next;
        } else {
            TrAccess lastFormalAccess0 = Tr_Access(p, (FAccess)formalFaccesses->data);
            formalAccesses = lastFormalAccess = DataList(lastFormalAccess0, NULL);
        }
        formalFaccesses = formalFaccesses->next;
    }
    p->formals = formalAccesses;
    p->locals = NULL;
    return p;
}

TrAccess Tr_Access(TrLevel level, FAccess faccess) {
    TrAccess p = CHECKED_MALLOC(struct sTrAccess);
    p->level = level;
    p->faccess = faccess;
    return p;
}

TrAccess Tr_AllocLocal(TrLevel level, bool escape) {
    FAccess fAccess = FrameAllocLocal(level->frame, escape);
    TrAccess access = Tr_Access(level, fAccess);

    if (level->locals) {
        List p = level->locals;
        while (p->next) {
            p = p->next;
        }
        p->next = DataList(access, NULL);
    } else {
        level->locals = DataList(access, NULL);
    }
    return access;
}

List Tr_Formals(TrLevel level) {
    return level->formals;
}
