#include "util.h"
#include "ir.h"
#include "codegen.h"
#include "assem.h"

static Frame codegen_frame = NULL;
static List instrList = NULL;
static List last = NULL;

static void emit(ASInstr instr);
static Temp munchExp(IrExpr expr);
static List munchArgs(unsigned int n, List eList,
								List formalAccesses);

static void emit(ASInstr instr) {
    if (!instrList) {
        instrList = last = DataList(instr, NULL);
    } else {
        last = last->next = DataList(instr, NULL);
    }
}

static void munchStmt(IrStmt stmt) {
	switch(stmt->kind) {
		case tIrMove: {
			IrExpr dst = stmt->as.move.dst, src = stmt->as.move.src;
			if (dst->kind == tIrMem)
				if (dst->as.mem->kind == tIrBinop &&
					dst->as.mem->as.binop.op == IR_PLUS &&
					dst->as.mem->as.binop.right->kind == tIrConst) {
						/* MOVE(MEM(BINOP(PLUS, e1, CONST(n))), e2) */
						IrExpr e1 = dst->as.mem->as.binop.left, e2 = src;
						int n = dst->as.mem->as.binop.right->as.const_;
						emit(AS_Move(String_format("mov [`s0 + %d],`s1\n", n),
							NULL, DataList(munchExp(e1), DataList(munchExp(e2), NULL))));
				} else if (dst->as.mem->kind == tIrBinop &&
					dst->as.mem->as.binop.op == IR_PLUS &&
					dst->as.mem->as.binop.left->kind == tIrConst) {
						/* MOVE(MEM(BINOP(PLUS, CONST(n), e1)), e2) */
						IrExpr e1 = dst->as.mem->as.binop.right, e2 = src;
						int n = dst->as.mem->as.binop.left->as.const_;
						emit(AS_Move(String_format("mov [`s0 + %d],`s1\n", n),
							NULL, DataList(munchExp(e1), DataList(munchExp(e2), NULL))));
				} else if (dst->as.mem->kind == tIrConst) {
					/* MOVE(MEM(CONST(n)), e2) */
				  IrExpr e2 = src;
					int n = dst->as.mem->as.const_;
					emit(AS_Move(String_format("mov [`s0 + %d],`s1\n", n),
							NULL, DataList(munchExp(e2), NULL)));
				} else if (src->kind == tIrMem) {
					/* MOVE(MEM(e1), MEM(e2)) */
					IrExpr e1 = dst->as.mem, e2 = src->as.mem;
					emit(AS_Move(String_format("mov [`s0],`s1\n"),
							NULL, DataList(munchExp(e1), DataList(munchExp(e2), NULL))));
				} else {
					/* MOVE(MEM(e1), e2) */
					IrExpr e1 = dst->as.mem, e2 = src;
					emit(AS_Move(String_format("mov [`s0],`s1\n"),
							NULL, DataList(munchExp(e1), DataList(munchExp(e2), NULL))));
				}
			else if (dst->kind == tIrTmp)
				/* MOVE(TEMP(e1), src) */
				emit(AS_Move(String_format("mov `d0,`s0\n"),
					DataList(munchExp(dst), NULL), DataList(munchExp(src), NULL)));
			else assert(0); /* destination of move must be temp or memory location */
			break;
		}
		case tIrSeq:
		{
			/* SEQ(stm1, stm2) */
			munchStmt((IrStmt)stmt->as.seq->data);
      munchStmt((IrStmt)stmt->as.seq->next->data);
			break;
		}
		case tIrLabel:
		{
			emit(AS_Label(String_format("%s:\n", LabelString(stmt->as.label)),
				stmt->as.label));
			break;
		}
		case tIrJump:
		{
			Temp r = munchExp(stmt->as.jump.expr);
			emit(AS_Oper(String_format("jmp `d0\n"), DataList(r, NULL), NULL,
				New_AS_Targets(stmt->as.jump.jumps)));
			break;
		}
		case tIrCjump:
		{
			Temp left = munchExp(stmt->as.cjump.left),
				right = munchExp(stmt->as.cjump.right);
			emit(AS_Oper("cmp `s0,`s1\n", NULL,
				DataList(left, DataList(right, NULL)), NULL));
			/* No need to deal with CJUMP false label
			 * as canonical module has it follow CJUMP */
			char *instr = NULL;
			switch (stmt->as.cjump.op) {
				case IR_EQ:
					instr = "je"; break;
				case IR_NE:
					instr = "jne"; break;
				case IR_LT:
					instr = "jl"; break;
				case IR_GT:
					instr = "jg"; break;
				case IR_LE:
					instr = "jle"; break;
				case IR_GE:
					instr = "jge"; break;
				default: assert(0);
			}
			emit(AS_Oper(String_format("%s `j0\n", instr), NULL, NULL,
				New_AS_Targets(DataList(stmt->as.cjump.t, NULL))));
			break;
		}
		case tIrExpr:
		{
			munchExp(stmt->as.expr);
			break;
		}
		default: assert(0);
	}
}

static Temp munchExp(IrExpr expr) {
	switch(expr->kind) {
		case tIrBinop: {
			char *op = NULL, *sign = NULL;
			switch (expr->as.binop.op) {
			case IR_PLUS:
				op = "add"; sign = "+"; break;
			case IR_MINUS:
				op = "sub"; sign = "-"; break;
			case IR_MUL:
				op = "mul"; sign = "*"; break;
			case IR_DIV:
				op = "div"; sign = "/"; break;
			default:
				assert(0 && "Invalid operator");
			}
      if (expr->as.binop.left->kind == tIrConst) {
          /* BINOP(op, CONST(i), e2) */
          Temp r = NewTemp();
          IrExpr e2 = expr->as.binop.right;
          int n = expr->as.binop.left->as.const_;
          emit(AS_Oper(String_format("%s `d0,`s0%s%d\n",
                      op, sign, n),
          DataList(r, NULL), DataList(munchExp(e2), NULL), NULL));
          return r;
      } else if (expr->as.binop.right->kind == tIrConst) {
          /* BINOP(op, e2, CONST(i)) */
          Temp r = NewTemp();
          IrExpr e2 = expr->as.binop.left;
          int n = expr->as.binop.right->as.const_;
          emit(AS_Oper(String_format("%s `d0,`s0%s%d\n",
                      op, sign, n),
          DataList(r, NULL), DataList(munchExp(e2), NULL), NULL));
          return r;
      } else {
          /* BINOP(op, e1, e2) */
          Temp r = NewTemp();
          IrExpr e1 = expr->as.binop.left, e2 = expr->as.binop.right;
          emit(AS_Oper(String_format("%s `d0,`s0%s`s1\n",
                      op, sign),
          DataList(r, NULL),
          DataList(munchExp(e1), DataList(munchExp(e2), NULL)), NULL));
          return r;
      }
      return NULL;
		}
		case tIrMem:
		{
			IrExpr loc = expr->as.mem;
			if (loc->kind == tIrBinop && loc->as.binop.op == IR_PLUS)
				if (loc->as.binop.left->kind == tIrConst) {
					/* MEM(BINOP(PLUS, CONST(i), e2)) */
					Temp r = NewTemp();
					IrExpr e2 = loc->as.binop.right;
					int n = loc->as.binop.left->as.const_;
					emit(AS_Move(String_format("mov `d0,[`s0+%d]\n", n),
						DataList(r, NULL), DataList(munchExp(e2), NULL)));
					return r;
				} else if (loc->as.binop.right->kind == tIrConst) {
					/* MEM(BINOP(PLUS, e2, CONST(i))) */
					Temp r = NewTemp();
					IrExpr e2 = loc->as.binop.left;
					int n = loc->as.binop.right->as.const_;
					emit(AS_Move(String_format("mov `d0,[`s0+%d]\n", n),
						DataList(r, NULL), DataList(munchExp(e2), NULL)));
					return r;
				} else assert(0);
			else if (loc->kind == tIrConst) {
				/* MEM(CONST(i)) */
				Temp r = NewTemp();
				int n = loc->as.const_;
				emit(AS_Move(String_format("mov `d0,[%d]\n", n), DataList(r, NULL),
					NULL));
				return r;
			} else {
				/* MEM(e1) */
				Temp r = NewTemp();
				IrExpr e1 = loc->as.mem;
				emit(AS_Move(String_format("mov `d0,[`s0]\n"), DataList(r, NULL),
					DataList(munchExp(e1), NULL)));
				return r;
			}
		}
		case tIrTmp:
		{
			/* TEMP(t) */
			return expr->as.tmp;
		}
		case tIrEseq:
		{
			/* ESEQ(e1, e2) */
			munchStmt(expr->as.eseq.stmt);
			return munchExp(expr->as.eseq.expr);
		}
		case tIrName:
		{
			/* NAME(n) */
			Temp r = NewTemp();
			/*Temp_enter(F_tempMap, r, Temp_labelstring(expr->u.NAME));*/
			return r;
		}
		case tIrConst:
		{
			/* CONST(i) */
			Temp r = NewTemp();
			emit(AS_Move(String_format("mov `d0,%d\n", expr->as.const_),
				DataList(r, NULL), NULL));
			return r;
		}
		case tIrCall:
		{
			/* CALL(fun, args) */
			Temp r = munchExp(expr->as.call.func);
			List list = munchArgs(0, expr->as.call.args,
											FrameFormals(codegen_frame));
			emit(AS_Oper("call `s0\n", Frame_caller_saves(), DataList(r, list), NULL));
			return r;
		}
		default: assert(0);
	}
}

static char *register_names[] = {"eax", "ebx", "ecx", "edx", "edi", "esi"};
static unsigned int reg_count = 0;
static List munchArgs(unsigned int n, List eList,
								List formals) {
	if (!codegen_frame) assert(0); // should never be NULL

	if (!eList || !formals) return NULL;

	// need first argument to be pushed onto stack last
	List tlist = munchArgs(n + 1, eList->next, formals->next);
	Temp e = munchExp((IrExpr)eList->data);
	/* use the frame here to determine whether we push
	 * or move into a register. */
	if (Frame_doesEscape((FAccess)formals->data)) {
		emit(AS_Oper("push `s0\n", NULL, DataList(e, NULL), NULL));
  } else {
		emit(AS_Move(
			String_format("mov %s,`s0", register_names[reg_count++]),
			NULL, DataList(e, NULL)));
	}
	return DataList(e, tlist);
}

List/*<ASInsn>*/ Codegen(Frame frame, List stmList/*<IrStmt>*/) {
    List asList = NULL;
    List sList = stmList;
    codegen_frame = frame; // set for munchArgs
    for (; sList; sList = sList->next) {
        munchStmt((IrStmt)sList->data);
    }
    asList = instrList;
    instrList = last = NULL;
    return asList;
}
