#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"
#include "errormsg.h"
#include "parse.h"

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
        fprintf(stdout, "Parsing succeeded\n");
    } else {
        fprintf(stderr, "Parsing failed\n");
    }
    return 0;
}
