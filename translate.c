#include <stdio.h>
#include "util.h"
#include "translate.h"
#include "frame.h"
#include "ir_pp.h"

static TrLevel _outermost = NULL;

static List _string_frags = NULL;
static List _proc_frags = NULL;

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

TrExpr Tr_SubscriptVar(TrExpr array, TrExpr index) {
    return Tr_Ex(Ir_Mem_Expr(
        Ir_Binop_Expr(
            IR_PLUS,
            Tr_UnEx(array),
            Ir_Binop_Expr(
                IR_MUL,
                Tr_UnEx(index),
                Ir_Const_Expr(FRAME_WORD_SIZE)
            )
        )
    ));
}

static void FillPatch(List patches, TempLabel label) {
    List p = patches;
    for (; p; p = p->next) {
        *((TempLabel *) p->data) = label;
    }
}

TrExpr Tr_IfExpr(TrExpr cond, TrExpr then, TrExpr else_) {
    TempLabel lt = NewLabel();
    TempLabel lf = NewLabel();
    TempLabel ldone = NewLabel();
    struct sTrCx cx = Tr_UnCx(cond);
    IrExpr result = Ir_Tmp_Expr(NewTemp());

    FillPatch(cx.trues, lt);
    FillPatch(cx.falses, lf);
    if (else_) {
        return Tr_Ex(Ir_Eseq_Expr(Ir_Seq_Stmt(vDataList(
                7,
                cx.stmt,
                Ir_Label_Stmt(lt),
                Ir_Move_Stmt(Ir_Mem_Expr(result), Tr_UnEx(then)),
                Ir_Jump_Stmt(Ir_Name_Expr(ldone), DataList(ldone, NULL)),
                Ir_Label_Stmt(lf),
                Ir_Move_Stmt(Ir_Mem_Expr(result), Tr_UnEx(else_)),
                Ir_Label_Stmt(ldone))),
            result));
    } else {
        return Tr_Nx(Ir_Seq_Stmt(vDataList(
              4,
              cx.stmt,
              Ir_Label_Stmt(lt),
              Tr_UnNx(then),
              Ir_Label_Stmt(lf))));
    }
}

TrExpr Tr_WhileExpr(TrExpr cond, TrExpr body) {
    TempLabel startl = NewLabel();
    TempLabel loopl = NewLabel();
    TempLabel donel = NewLabel();
    struct sTrCx cx = Tr_UnCx(cond);

    FillPatch(cx.trues, loopl);
    FillPatch(cx.falses, donel);
    return Tr_Nx(Ir_Seq_Stmt(vDataList(
          6,
          Ir_Label_Stmt(startl),
          cx.stmt,
          Ir_Label_Stmt(loopl),
          Tr_UnNx(body),
          Ir_Jump_Stmt(Ir_Name_Expr(startl), DataList(startl, NULL)),
          Ir_Label_Stmt(donel))));
}

TrExpr Tr_ForExpr(TrAccess access, TrExpr lo, TrExpr hi, TrExpr body) {
    IrExpr var = Frame_Expr(access->faccess, Ir_Tmp_Expr(Frame_fp()));
    TempLabel startl = NewLabel();
    TempLabel loopl = NewLabel();
    TempLabel donel = NewLabel();
    IrStmt cond = Ir_Cjump_Stmt(IR_LE, var, Tr_UnEx(hi), loopl, donel);
    return Tr_Nx(Ir_Seq_Stmt(vDataList(
          7,
          Ir_Move_Stmt(var, Tr_UnEx(lo)),
          Ir_Label_Stmt(startl),
          cond,
          Ir_Label_Stmt(loopl),
          Tr_UnNx(body),
          Ir_Move_Stmt(var, Ir_Binop_Expr(IR_PLUS, var, Ir_Const_Expr(1))),
          Ir_Label_Stmt(donel))));
}

static TrAccess Tr_StaticLink(TrLevel level) {
    assert(level);
    return level->formals->data;
}

TrExpr Tr_CallExpr(TrLevel level, TempLabel label, List/*<TrExpr>*/ args) {
    IrExpr func = Ir_Name_Expr(label);
    IrExpr fp = Ir_Const_Expr(Frame_Offset(Tr_StaticLink(level)->faccess));
    List l_args = DataList(fp, NULL);
    List l_next = l_args;
    for (; args; args = args->next) {
        l_next = l_next->next = DataList(Tr_UnEx(args->data), NULL);
    }
    return Tr_Ex(Ir_Call_Expr(func, l_args));
}

// expects `exprs` in reverse order, with result in head position
// zips them up to create a tree of EseqExprs, in correct order (reverse of given)
TrExpr Tr_SeqExpr(List/*<TrExpr>*/ exprs) {
	IrExpr result = Tr_UnEx((TrExpr)exprs->data);
  List p;
	for (p = exprs->next; p; p = p->next) {
		result = Ir_Eseq_Expr(Ir_Expr_Stmt(Tr_UnEx((TrExpr)p->data)), result);
  }
	return Tr_Ex(result);
}

TrExpr Tr_NoExp(void) {
	return Tr_Ex(Ir_Const_Expr(0));
}

TrExpr Tr_StringExpr(string str) {
    TempLabel label = NewLabel();
    Frag frag = String_Frag(label, str);
    Add_Frag(frag);
    return Tr_Ex(Ir_Name_Expr(label));
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

// TrExpr -> IrExpr
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

// TrExpr -> IrStmt
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

// TrExpr -> struct sTrCx
struct sTrCx Tr_UnCx(TrExpr expr) {
    struct sTrCx cx = { 0 };

    switch (expr->kind) {
        // if expr is 0, jump
        case tTrEx:
            cx.stmt = Ir_Cjump_Stmt(
              IR_EQ, expr->as.ex, Ir_Const_Expr(0), NULL, NULL);
            cx.trues = DataList(&(cx.stmt->as.cjump.t), NULL);
            cx.falses = DataList(&(cx.stmt->as.cjump.f), NULL);
            return cx;
        case tTrCx:
            return expr->as.cx;
        case tTrNx:
            assert(0);
    }

    assert(0);
}

void Tr_PPExpr(TrExpr expr) {
    Ir_PP_Stmts(stdout, DataList(Tr_UnNx(expr), NULL));
}

void Tr_PPExprs(List/*<TrExpr>*/ exprs) {
    while (exprs) {
        Tr_PPExpr((TrExpr)exprs->data);
        exprs = exprs->next;
    }
}

// Returns list of fragments, strings then procs.
// NOTE: only call this once!
List/*<Frag>*/ Tr_getResult(void) {
    List cursor = NULL, prev = NULL;
    for (cursor = _string_frags; cursor; cursor = cursor->next) {
        prev = cursor;
    }
    if (prev) prev->next = _proc_frags;
    return _string_frags ? _string_frags : _proc_frags;
}

Frag String_Frag(TempLabel label, string str) {
    Frag p = CHECKED_MALLOC(struct sFrag);
    p->kind = tStringFrag;
    p->as.str.label = label;
    p->as.str.str = str;
    return p;
}

Frag Proc_Frag(IrStmt stmt, Frame frame) {
    Frag p = CHECKED_MALLOC(struct sFrag);
    p->kind = tProcFrag;
    p->as.proc.stmt = stmt;
    p->as.proc.frame = frame;
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

