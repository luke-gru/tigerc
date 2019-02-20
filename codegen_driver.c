#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"
#include "errormsg.h"
#include "parse.h"
#include "print_ast.h"
#include "semantics.h"
#include "frame.h"
#include "codegen.h"
#include "canon.h"
#include "assem.h"

extern int yyparse(void);
extern int yydebug;

static void GenCode(List frags, FILE *out) {
    List p;
    for (p = frags; p; p = p->next) {
        Frag frag = (Frag)p->data;
        if (frag->kind == tStringFrag) {
            /*fprintf(out, "STRING FRAGMENTS:\n");*/
            /*fprintf(out, "    %s: \"%s\"\n",*/
                    /*LabelString(frag->as.str.label),*/
                    /*frag->as.str.str);*/
        } else {
            fprintf(out, "    %s:\n", LabelString(frag->as.proc.name));
            List stmtList = C_Linearize(frag->as.proc.stmt);
            stmtList = C_TraceSchedule(C_BasicBlocks(stmtList));
            assert(stmtList);
            List asList = Codegen(frag->as.proc.frame, stmtList);
            assert(asList);
            TempTable nameMap = Temp_NameMap();
            AS_print_instrs(out, asList, nameMap);
            fprintf(out, "\n");
        }
    }
}

int main(int argc, char **argv) {
    /*yydebug = 1;*/
    if (argc!=2) {
        fprintf(stderr,"usage: a.out filename\n");
        exit(1);
    }
    N_Expr expr = parse(argv[1]);
    if (expr) {
        pr_exp(stderr, expr, -1);
        fprintf(stdout, "\nParsing succeeded\n");
        List frags = TypeCheck(expr);
        if (EM_errors == 0) {
            fprintf(stdout, "Type check succeeded\n");
            GenCode(frags, stdout);
        } else {
            fprintf(stderr, "Type check failed\n");
        }
    } else {
        fprintf(stderr, "Parsing failed\n");
    }
    return 0;
}
