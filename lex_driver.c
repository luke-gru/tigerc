#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "errormsg.h"
#include "tokens.h"

YYSTYPE yylval;

int yylex(void); /* prototype for the lexing function */

int main(int argc, char **argv) {
    string fname; int tok;
    if (argc!=2) {
        fprintf(stderr,"usage: a.out FILENAME|-\n");
        exit(1);
    }
    fname = argv[1];
    if (strcmp(fname, "-") == 0) {
        EM_fset(stdin, "stdin");
    } else {
        EM_reset(fname);
    }
    for(;;) {
        tok=yylex();
        if (tok==0) break;
        switch(tok) {
            case TK_ID: case TK_STRING:
                printf("%10s %4d %s\n",tokname(tok),EM_tokPos,yylval.str);
                break;
            case TK_INT:
                printf("%10s %4d %d\n",tokname(tok),EM_tokPos,yylval.num);
                break;
            default:
                printf("%10s %4d\n",tokname(tok),EM_tokPos);
        }
    }
    return 0;
}
