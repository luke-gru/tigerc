#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"
#include "errormsg.h"
#include "parse.h"
#include "print_ast.h"
#include "semantics.h"
#include "frame.h"

extern int yyparse(void);
extern int yydebug;

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
        ExprTy tyCheckRes = TypeCheck(expr);
        (void)tyCheckRes;
        if (EM_errors == 0) {
            fprintf(stdout, "Type check succeeded\n");
            PP_Frags(stdout);
            Tr_PPExpr(tyCheckRes.trExpr);
        } else {
            fprintf(stderr, "Type check failed\n");
        }
    } else {
        fprintf(stderr, "Parsing failed\n");
    }
    return 0;
}
