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

TrExpr Tr_NumExpr(int num) {
    return Tr_Ex(Ir_Const_Expr(num));
}

TrExpr Tr_SimpleVar(TrAccess access, TrLevel level) {
    IrExpr fp = Ir_Tmp_Expr(Frame_fp());

    return Tr_Ex(Frame_Expr(access->faccess, fp));
}

TrExpr Tr_AssignExpr(TrExpr lhs, TrExpr rhs) {
    return Tr_Nx(Ir_Move_Stmt(Tr_UnEx(lhs), Tr_UnEx(rhs)));
}

TrExpr Tr_ArrayExpr(TrExpr size, TrExpr init) {
    return Tr_Ex(Frame_ExternalCall(
        "_InitArray", vDataList(2, Tr_UnEx(size), Tr_UnEx(init))));
}

static void FillPatch(List patches, TempLabel label) {
    List p = patches;
    for (; p; p = p->next) {
        *((TempLabel *) p->data) = label;
    }
}

TrExpr Tr_Ex(IrExpr irExpr) {
    TrExpr p = CHECKED_MALLOC(struct sTrExpr);
    p->kind = tTrEx;
    p->as.ex = irExpr;
    return p;
}

TrExpr Tr_Nx(IrStmt irStmt) {
    TrExpr p = CHECKED_MALLOC(struct sTrExpr);
    p->kind = tTrNx;
    p->as.nx = irStmt;
    return p;
}

TrExpr Tr_Cx(TrCx cx) {
    TrExpr p = CHECKED_MALLOC(struct sTrExpr);
    p->kind = tTrCx;
    p->as.cx = cx;
    return p;
}

// turn TrExpr -> IrExpr
IrExpr Tr_UnEx(TrExpr trExpr) {
    switch (trExpr->kind) {
        case tTrEx:
            return trExpr->as.ex;
        case tTrNx:
            return Ir_Eseq_Expr(trExpr->as.nx, Ir_Const_Expr(0));
        case tTrCx: {
            Temp tmp = NewTemp();
            TempLabel t = NewLabel();
            TempLabel f = NewLabel();
            FillPatch(trExpr->as.cx->trues, t);
            FillPatch(trExpr->as.cx->falses, f);
            return Ir_Eseq_Expr(
              Ir_Seq_Stmt(vDataList(
                  5,
                  Ir_Move_Stmt(Ir_Tmp_Expr(tmp),
                               Ir_Const_Expr(1)),
                  trExpr->as.cx->stmt,
                  Ir_Label_Stmt(f),
                  Ir_Move_Stmt(Ir_Tmp_Expr(tmp),
                               Ir_Const_Expr(0)),
                  Ir_Label_Stmt(t))),
              Ir_Tmp_Expr(tmp));
        default:
            assert(0);
        }
    }
}
