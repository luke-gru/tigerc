#include "frame.h"

#define FRAME_ARGS_REG 4
const int FRAME_WORD_SIZE = 4;

struct sFrame {
    TempLabel name;
    List/*<FAccess>*/ formals;
    List/*<FAccess>*/ locals;
    int localCount;
};

struct sFAccess {
    enum { tAccessFrame, tAccessReg } kind;
    union {
        int offset; // frame offset
        Temp reg; // reg
    } as;
};

static FAccess F_AccessFrame(int offset) {
    FAccess p = CHECKED_MALLOC(struct sFAccess);
    p->kind = tAccessFrame;
    p->as.offset = offset;
    return p;
}

static FAccess F_AccessReg(Temp temp) {
    FAccess p = CHECKED_MALLOC(struct sFAccess);
    p->kind = tAccessReg;
    p->as.reg = temp;
    return p;
}

Frame NewFrame(TempLabel name, List formalEscapes) {
    Frame f = CHECKED_MALLOC(struct sFrame);
    f->name = name;
    List formalAccesses = NULL;
    List formalAccessesLast = NULL;
    int i = 0;
    while (formalEscapes) {
        FAccess faccess;
        if (formalAccesses->b || i >= FRAME_ARGS_REG) {
            faccess = F_AccessFrame(i*FRAME_WORD_SIZE);
        } else {
            faccess = F_AccessReg(NewTemp());
        }
        if (formalAccesses) {
            formalAccessesLast->next = DataList(faccess, NULL);
            formalAccessesLast = formalAccessesLast->next;
        } else {
            formalAccesses = DataList(faccess, NULL);
            formalAccessesLast = formalAccesses;
        }
        formalEscapes = formalEscapes->next;
    }
    f->formals = formalAccesses;
    f->locals = NULL;
    f->localCount = 0;
    return f;
}

