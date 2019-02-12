#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"
#include "errormsg.h"
#include "parse.h"

extern int yyparse(void);

N_Expr parse(string fname) {
    if (strcmp(fname, "-") == 0) {
        EM_fset(stdin, "stdin");
    } else {
        EM_reset(fname);
    }
    if (yyparse() == 0) { /* parsing worked */
        fprintf(stderr,"Parsing successful!\n");
    } else {
        fprintf(stderr,"Parsing failed\n");
    }
    return NULL;
}


int main(int argc, char **argv) {
    if (argc!=2) {
        fprintf(stderr,"usage: a.out filename\n");
        exit(1);
    }
    parse(argv[1]);
    return 0;
}
