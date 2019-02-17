#include <stdio.h>
#include <assert.h>
#include "ir_pp.h"
#include "ir.h"
#include "temp.h"

static void pp_expr(FILE *out, int d, IrExpr expr);

static void indent(FILE *out, int d) {
    int i;

    for (i = 0; i <= d; ++i) {
        fprintf(out, "    ");
    }
}

static char const* binops[] = {
    "PLUS",
    "MINUS",
    "TIMES",
    "DIVIDE",
    "AND",
    "OR",
    "XOR",
    "LSHIFT",
    "RSHIFT",
    "ARSHIFT",
};

static char const* relops[] = {
    "EQ",
    "NE",
    "LT",
    "LE",
    "GT",
    "GE",
    "ULT",
    "ULE",
    "UGT",
    "UGE",
};

static void pp_stmt(FILE *out, int d, IrStmt stmt) {
    switch (stmt->kind) {
        case tIrSeq: {
            List p;

            indent(out, d);
            fprintf(out, "SEQ(\n");
            for (p = stmt->as.seq; p; p = p->next) {
                pp_stmt(out, d + 1, p->data);
            }
            indent(out, d);
            fprintf(out, ")\n");
            break;
        }

        case tIrLabel:
            indent(out, d);
            fprintf(out, "LABEL %s\n", LabelString(stmt->as.label));
            break;

        case tIrJump:
            indent(out, d);
            fprintf(out, "JUMP(\n");
            pp_expr(out, d + 1, stmt->as.jump.expr);
            indent(out, d);
            fprintf(out, ")\n");
            break;

        case tIrCjump:
            indent(out, d);
            fprintf(out, "CJUMP(%s\n", relops[stmt->as.cjump.op]);
            pp_expr(out, d + 1, stmt->as.cjump.left);
            pp_expr(out, d + 1, stmt->as.cjump.right);
            indent(out, d + 1);
            fprintf(out, "%s, %s)\n",
                    LabelString(stmt->as.cjump.t),
                    LabelString(stmt->as.cjump.f));
            break;

        case tIrMove:
            indent(out, d);
            fprintf(out, "MOVE(\n");
            pp_expr(out, d + 1, stmt->as.move.dst);
            pp_expr(out, d + 1, stmt->as.move.src);
            indent(out, d);
            fprintf(out, ")\n");
            break;

        case tIrExpr:
            indent(out, d);
            fprintf(out, "EXPR(\n");
            pp_expr(out, d + 1, stmt->as.expr);
            indent(out, d);
            fprintf(out, ")\n");
            break;

        default:
            assert(false);
    }
}

void pp_expr(FILE *out, int d, IrExpr expr)
{
    switch (expr->kind) {
        case tIrBinop:
            indent(out, d);
            fprintf(out, "BINOP(%s\n", binops[expr->as.binop.op]);
            pp_expr(out, d + 1, expr->as.binop.left);
            pp_expr(out, d + 1, expr->as.binop.right);
            indent(out, d);
            fprintf(out, ")\n");
            break;

        case tIrMem:
            indent(out, d);
            fprintf(out, "MEM(\n");
            pp_expr(out, d + 1, expr->as.mem);
            indent(out, d);
            fprintf(out, ")\n");
            break;

        case tIrTmp:
            indent(out, d);
            fprintf(out, "TEMP t%d\n", expr->as.tmp->num);
            break;

        case tIrEseq:
            indent(out, d);
            fprintf(out, "ESEQ(\n");
            pp_stmt(out, d + 1, expr->as.eseq.stmt);
            pp_expr(out, d + 1, expr->as.eseq.expr);
            indent(out, d);
            fprintf(out, ")\n");
            break;

        case tIrName:
            indent(out, d);
            fprintf(out, "NAME %s\n", LabelString(expr->as.name));
            break;

        case tIrConst:
            indent(out, d);
            fprintf(out, "CONST %d\n", expr->as.const_);
            break;

        case tIrCall:
        {
            List p;

            indent(out, d);
            fprintf(out, "CALL(\n");
            pp_expr(out, d + 1, expr->as.call.func);
            for (p = expr->as.call.args; p; p = p->next) {
                pp_expr(out, d + 1, p->data);
            }
            indent(out, d);
            fprintf(out, ")\n");
            break;
        }

        default:
            assert(0);
    }
}

void Ir_PP_Stmts(FILE *out, List stmts) {
    for (; stmts; stmts = stmts->next) {
        assert(out);
        pp_stmt(out, 0, stmts->data);
    }
}
