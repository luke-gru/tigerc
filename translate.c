#include <stdio.h>
#include "util.h"
#include "translate.h"
#include "frame.h"
#include "ir_pp.h"

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

TrExpr Tr_BinopExpr(IrBinop op, TrExpr left, TrExpr right) {
    IrExpr l = Tr_UnEx(left);
    IrExpr r = Tr_UnEx(right);
    return Tr_Ex(Ir_Binop_Expr(op, l, r));
}

TrExpr Tr_RelopExpr(IrRelop op, TrExpr left, TrExpr right) {
    IrStmt stmt = Ir_Cjump_Stmt(op, Tr_UnEx(left), Tr_UnEx(right), NULL, NULL);
    return Tr_Cx(DataList(&stmt->as.cjump.t, NULL),
                 DataList(&stmt->as.cjump.f, NULL),
                 stmt);
}

TrExpr Tr_FieldVar(TrExpr recExpr, int fieldIdx) {
    return Tr_Ex(Ir_Mem_Expr(Ir_Binop_Expr(
          IR_PLUS,
          Tr_UnEx(recExpr),
          Ir_Binop_Expr(IR_MUL,
                        Ir_Const_Expr(fieldIdx),
                        Ir_Const_Expr(FRAME_WORD_SIZE)))));
}

// allocate new record, initialize fields
TrExpr Tr_RecordExpr(List/*<TrExpr>*/ fieldExprs, int fieldSize) {
    IrExpr addr = Ir_Tmp_Expr(NewTemp());
    IrExpr alloc = Frame_ExternalCall(
      "_Alloc", DataList(Ir_Const_Expr(fieldSize * FRAME_WORD_SIZE), NULL));
    List p, q = NULL, r = NULL;
    int i;

    for (p = fieldExprs, i = 0; p; p = p->next, i++) {
        TrExpr fieldExpr = (TrExpr)p->data;
        IrExpr offset = Ir_Binop_Expr(IR_PLUS, addr,
                                         Ir_Const_Expr(FRAME_WORD_SIZE * i));
        IrStmt moveStmt = Ir_Move_Stmt(Ir_Mem_Expr(offset), Tr_UnEx(fieldExpr));
        List next = DataList(moveStmt, NULL);
        if (q) {
            r->next = next;
            r = next;
        } else {
            q = r = next;
        }
    }
    return Tr_Ex(
      Ir_Eseq_Expr(
        Ir_Seq_Stmt(DataList(Ir_Move_Stmt(addr, alloc), q)),
        addr));
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

TrExpr Tr_Cx(List trues, List falses, IrStmt stmt) {
    TrExpr p = CHECKED_MALLOC(struct sTrExpr);
    p->kind = tTrCx;
    p->as.cx.trues = trues;
    p->as.cx.falses = falses;
    p->as.cx.stmt = stmt;
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
            FillPatch(trExpr->as.cx.trues, t);
            FillPatch(trExpr->as.cx.falses, f);
            return Ir_Eseq_Expr(
              Ir_Seq_Stmt(vDataList(
                  5,
                  Ir_Move_Stmt(Ir_Tmp_Expr(tmp),
                               Ir_Const_Expr(1)),
                  trExpr->as.cx.stmt,
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

IrStmt Tr_UnNx(TrExpr trExpr) {
    switch (trExpr->kind) {
        case tTrNx:
           return trExpr->as.nx;
        case tTrEx:
           return Ir_Expr_Stmt(trExpr->as.ex);
        case tTrCx:
           assert(0); // TODO
    }
    assert(0);
}

void Tr_PPExpr(TrExpr expr) {
    Ir_PP_Stmts(stdout, DataList(Tr_UnNx(expr), NULL));
}
