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
        FAccess faccess = NULL;
        if (formalEscapes->b || i >= FRAME_ARGS_REG) {
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
        i++;
    }
    f->formals = formalAccesses;
    f->locals = NULL;
    f->localCount = 0;
    return f;
}

FAccess FrameAllocLocal(Frame fr, bool escape) {
    FAccess faccess;

    if (escape) {
        fr->localCount++;
        /* -2 for the the return address and frame pointer. */
        faccess = F_AccessFrame(FRAME_WORD_SIZE * (-2 - fr->localCount));
    } else {
        faccess = F_AccessReg(NewTemp());
    }
    if (fr->locals) {
        List p = fr->locals;
        while (p->next) {
            p = p->next;
        }
        p->next = DataList(faccess, NULL);
    } else {
        fr->locals = DataList(faccess, NULL);
    }
    return faccess;
}

List FrameFormals(Frame fr) {
    return fr->formals;
}
