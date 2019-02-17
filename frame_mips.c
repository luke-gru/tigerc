#include "util.h"
#include "frame.h"
#include "ir.h"

#define FRAME_ARGS_REG 4
const int FRAME_WORD_SIZE = 4;

static List _string_frags = NULL;
static List _proc_frags = NULL;

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

// frame pointer always in same temp
Temp Frame_fp(void) {
    static Temp _fp = NULL;

    if (!_fp) {
        _fp = NewTemp();
    }
    return _fp;
}

// return IR for frame access relative to `fp` if in memory,
// or the register itself if in register.
IrExpr Frame_Expr(FAccess faccess, IrExpr fp) {
    switch (faccess->kind) {
        case tAccessFrame:
            return Ir_Mem_Expr(Ir_Binop_Expr(
                IR_PLUS,
                Ir_Const_Expr(faccess->as.offset),
                fp));

        case tAccessReg:
            return Ir_Tmp_Expr(faccess->as.reg);
    }

    assert(0);
    return NULL;
}

IrExpr Frame_ExternalCall(string fnName, List/*<IrExpr>*/ args) {
    return Ir_Call_Expr(Ir_Name_Expr(NamedLabel(fnName)), args);
}

int Frame_Offset(FAccess faccess) {
    assert(faccess && faccess->kind == tAccessFrame);
    return faccess->as.offset;
}

Frag String_Frag(TempLabel label, string str) {
    Frag p = CHECKED_MALLOC(struct sFrag);
    p->kind = tStringFrag;
    p->as.str.label = label;
    p->as.str.str = str;
    return p;
}

void Add_Frag(Frag frag) {
    switch (frag->kind) {
        case tStringFrag:
            _string_frags = DataListAppend(_string_frags, frag);
            break;
        case tProcFrag:
            _proc_frags = DataListAppend(_proc_frags, frag);
            break;
        default:
            assert(false);
    }
}
