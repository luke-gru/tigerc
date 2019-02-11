%{
#include <string.h>
#include "util.h"
#include "tokens.h"
#include "errormsg.h"

int charPos=1;

int yywrap(void) {
  charPos=1;
  return 1;
}

void adjust(void) {
  EM_tokPos=charPos;
  charPos+=yyleng;
}

%}

digits  [0-9]+

%%
" "    { adjust(); continue; }
"+"    { adjust(); return PLUS; }
"-"    { adjust(); return MINUS; }
"*"    { adjust(); return TIMES; }
"/"    { adjust(); return DIVIDE; }
","    { adjust(); return COMMA; }
";"    { adjust(); return SEMICOLON; }
"("    { adjust(); return LPAREN; }
")"    { adjust(); return RPAREN; }
"=="   { adjust(); return  EQ; }
"="    { adjust(); return ASSIGN; }
for    { adjust(); return FOR; }
\n     { adjust(); EM_newline(); continue; }
[a-z][a-z0-9]* { adjust();
                yylval.sval = String(yytext);
                return ID;
               }
{digits}  {
            adjust();
            yylval.ival = atoi(yytext);
            return INT;
          }
("--"[a-z]*"\n")|("\t")+    { adjust(); }
.      { adjust(); EM_error(EM_tokPos, "illegal token"); }
