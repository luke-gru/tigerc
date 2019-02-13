#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "ast.h"  /* abstract syntax data structures */
#include "print_ast.h" /* function prototype */

/* local function prototypes */
static void pr_var(FILE *out, N_Var v, int d);
static void pr_dec(FILE *out, N_Decl v, int d);
static void pr_ty(FILE *out, N_Type v, int d);
static void pr_field(FILE *out, N_Field v, int d);
static void pr_fundec(FILE *out, N_FunDecl v, int d);
static void pr_namety(FILE *out, N_NameType v, int d);
static void pr_efield(FILE *out, N_EField v, int d);

static void pr_decList(FILE *out, List v, int d);
static void pr_efieldList(FILE *out, List v, int d);
static void pr_nametyList(FILE *out, List v, int d);
static void pr_fundecList(FILE *out, List v, int d);
static void pr_expList(FILE *out, List v, int d);
static void pr_fieldList(FILE *out, List v, int d);

static void indent(FILE *out, int d) {
    for (int i = 0; i <= d; i++) fprintf(out, " ");
}

/* Print N_Var types. Indent d spaces. */
static void pr_var(FILE *out, N_Var v, int d) {
    indent(out, d);
    switch (v->kind) {
    case tSimpleVar:
        fprintf(out, "simpleVar(%s)", SymName(v->as.simple));
        break;
    case tFieldVar:
         fprintf(out, "%s\n", "fieldVar(");
         pr_var(out, v->as.field.var, d+1);
         fprintf(out, "%s\n", ",");
         indent(out, d+1);
         fprintf(out, "%s)", SymName(v->as.field.sym));
         break;
    case tSubscriptVar:
        fprintf(out, "%s\n", "subscriptVar(");
        pr_var(out, v->as.subscript.var, d+1);
        fprintf(out, "%s\n", ",");
        pr_exp(out, v->as.subscript.expr, d+1);
        fprintf(out, "%s", ")");
        break;
    default:
        assert(0);
    }
}

static char str_oper[][12] = {
   "PLUS", "MINUS", "TIMES", "DIVIDE",
   "EQUAL", "NOTEQUAL", "LESSTHAN", "LESSEQ", "GREAT", "GREATEQ"
};

static void pr_oper(FILE *out, Op d) {
  fprintf(out, "%s", str_oper[d]);
}

/* Print A_var types. Indent d spaces. */
void pr_exp(FILE *out, N_Expr v, int d) {
    indent(out, d);
    switch (v->kind) {
        case tVarExpr:
            fprintf(out, "varExp(\n"); pr_var(out, v->as.var, d+1);
            fprintf(out, "%s", ")");
            break;
        case tNilExpr:
            fprintf(out, "nilExp()");
            break;
        case tIntExpr:
            fprintf(out, "intExp(%d)", v->as.intVal);
            break;
        case tStringExpr:
            fprintf(out, "stringExp(%s)", v->as.stringVal);
            break;
        case tCallExpr:
            fprintf(out, "callExp(%s,\n", SymName(v->as.call.func));
            pr_expList(out, v->as.call.args, d+1);
            fprintf(out, ")");
            break;
        case tOpExpr:
            fprintf(out, "opExp(\n");
            indent(out, d+1);
            pr_oper(out, v->as.op.op);
            fprintf(out, ",\n");
            pr_exp(out, v->as.op.left, d+1);
            fprintf(out, ",\n");
            pr_exp(out, v->as.op.right, d+1);
            fprintf(out, ")");
            break;
        case tRecordExpr:
            fprintf(out, "recordExp(%s,\n", SymName(v->as.record.ty));
            pr_efieldList(out, v->as.record.fields, d+1);
            fprintf(out, ")");
            break;
        case tSeqExpr:
            fprintf(out, "seqExp(\n");
            pr_expList(out, v->as.seq, d+1);
            fprintf(out, ")");
            break;
        case tAssignExpr:
            fprintf(out, "assignExp(\n");
            pr_var(out, v->as.assign.var, d+1); fprintf(out, ",\n");
            pr_exp(out, v->as.assign.expr, d+1); fprintf(out, ")");
            break;
        case tIfExpr:
            fprintf(out, "iffExp(\n");
            pr_exp(out, v->as.iff.test, d+1); fprintf(out, ",\n");
            pr_exp(out, v->as.iff.then, d+1);
            if (v->as.iff.elsee) { /* else is optional */
                fprintf(out, ",\n");
                pr_exp(out, v->as.iff.elsee, d+1);
            }
            fprintf(out, ")");
            break;
        case tWhileExpr:
            fprintf(out, "whileExp(\n");
            pr_exp(out, v->as.whilee.test, d+1); fprintf(out, ",\n");
            pr_exp(out, v->as.whilee.body, d+1); fprintf(out, ")\n");
            break;
        case tForExpr:
            fprintf(out, "forExp(%s,\n", SymName(v->as.forr.var));
            pr_exp(out, v->as.forr.lo, d+1); fprintf(out, ",\n");
            pr_exp(out, v->as.forr.hi, d+1); fprintf(out, "%s\n", ",");
            pr_exp(out, v->as.forr.body, d+1); fprintf(out, ",\n");
            indent(out, d+1); fprintf(out, "%s", v->as.forr.escape ? "TRUE)" : "FALSE)");
            break;
        case tBreakExpr:
            fprintf(out, "breakExp()");
            break;
        case tLetExpr:
            fprintf(out, "letExp(\n");
            pr_decList(out, v->as.let.decls, d+1); fprintf(out, ",\n");
            pr_exp(out, v->as.let.body, d+1); fprintf(out, ")");
            break;
        case tArrayExpr:
            fprintf(out, "arrayExp(%s,\n", SymName(v->as.array.ty));
            pr_exp(out, v->as.array.size, d+1); fprintf(out, ",\n");
            pr_exp(out, v->as.array.init, d+1); fprintf(out, ")");
            break;
        default:
            assert(0);
    }
}

static void pr_dec(FILE *out, N_Decl v, int d) {
    indent(out, d);
    switch (v->kind) {
    case tFunctionDecl:
        fprintf(out, "functionDec(\n");
        pr_fundecList(out, v->as.functions, d+1); fprintf(out, ")");
        break;
    case tVarDecl:
        fprintf(out, "varDec(%s,\n", SymName(v->as.var.var));
        if (v->as.var.ty) {
            indent(out, d+1); fprintf(out, "%s,\n", SymName(v->as.var.ty));
        }
        pr_exp(out, v->as.var.init, d+1); fprintf(out, ",\n");
        indent(out, d+1); fprintf(out, "%s", v->as.var.escape ? "TRUE)" : "FALSE)");
        break;
    case tTypesDecl:
        fprintf(out, "typeDec(\n");
        pr_nametyList(out, v->as.types, d+1); fprintf(out, ")");
        break;
    default:
        assert(0);
    }
}

static void pr_ty(FILE *out, N_Type v, int d) {
    indent(out, d);
    switch (v->kind) {
    case tNameTy:
        fprintf(out, "nameTy(%s)", SymName(v->as.name));
        break;
    case tRecordTy:
        fprintf(out, "recordTy(\n");
        pr_fieldList(out, v->as.record, d+1); fprintf(out, ")");
        break;
    case tArrayTy:
        fprintf(out, "arrayTy(%s)", SymName(v->as.array));
        break;
    default:
        assert(0);
    }
}

static void pr_field(FILE *out, N_Field v, int d) {
    indent(out, d);
    fprintf(out, "field(%s,\n", SymName(v->name));
    indent(out, d+1); fprintf(out, "%s,\n", SymName(v->ty));
    indent(out, d+1); fprintf(out, "%s", v->escape ? "TRUE)" : "FALSE)");
}

static void pr_fieldList(FILE *out, List v, int d) {
    indent(out, d);
    if (v) {
        fprintf(out, "fieldList(\n");
        pr_field(out, (N_Field)v->data, d+1); fprintf(out, ",\n");
        pr_fieldList(out, v->next, d+1); fprintf(out, ")");
    } else { fprintf(out, "fieldList()"); }
}

static void pr_expList(FILE *out, List v, int d) {
    indent(out, d);
    if (v) {
        fprintf(out, "expList(\n");
        pr_exp(out, (N_Expr)v->data, d+1); fprintf(out, ",\n");
        pr_expList(out, v->next, d+1);
        fprintf(out, ")");
    } else { fprintf(out, "expList()"); }
}

static void pr_fundec(FILE *out, N_FunDecl v, int d) {
    indent(out, d);
    fprintf(out, "fundec(%s,\n", SymName(v->name));
    pr_fieldList(out, v->params, d+1); fprintf(out, ",\n");
    if (v->result) {
        indent(out, d+1); fprintf(out, "%s,\n", SymName(v->result));
    }
    pr_exp(out, v->body, d+1); fprintf(out, ")");
}

static void pr_fundecList(FILE *out, List v, int d) {
    indent(out, d);
    if (v) {
        fprintf(out, "fundecList(\n");
        pr_fundec(out, (N_FunDecl)v->data, d+1); fprintf(out, ",\n");
        pr_fundecList(out, v->next, d+1); fprintf(out, ")");
    } else { fprintf(out, "fundecList()"); }
}

static void pr_decList(FILE *out, List v, int d) {
    indent(out, d);
    if (v) {
        fprintf(out, "decList(\n");
        pr_dec(out, (N_Decl)v->data, d+1); fprintf(out, ",\n");
        pr_decList(out, v->next, d+1);
        fprintf(out, ")");
    } else { fprintf(out, "decList()"); }

}

static void pr_namety(FILE *out, N_NameType v, int d) {
    indent(out, d);
    fprintf(out, "namety(%s,\n", SymName(v->name));
    pr_ty(out, v->ty, d+1); fprintf(out, ")");
}

static void pr_nametyList(FILE *out, List v, int d) {
    indent(out, d);
    if (v) {
        fprintf(out, "nametyList(\n");
        pr_namety(out, (N_NameType)v->data, d+1); fprintf(out, ",\n");
        pr_nametyList(out, v->next, d+1); fprintf(out, ")");
    }
    else fprintf(out, "nametyList()");
}

static void pr_efield(FILE *out, N_EField v, int d) {
    indent(out, d);
    if (v) {
        fprintf(out, "efield(%s,\n", SymName(v->name));
        pr_exp(out, v->expr, d+1); fprintf(out, ")");
    }
    else fprintf(out, "efield()");
}

static void pr_efieldList(FILE *out, List v, int d) {
    indent(out, d);
    if (v) {
        fprintf(out, "efieldList(\n");
        pr_efield(out, (N_EField)v->data, d+1); fprintf(out, ",\n");
        pr_efieldList(out, v->next, d+1); fprintf(out, ")");
    }
    else fprintf(out, "efieldList()");
}

