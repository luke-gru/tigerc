%{

/*#define YY_NO_UNISTD_H*/

#include <ctype.h>
#include <string.h>

#include "util.h"
#include "tokens.h"
#include "errormsg.h"

static int _char_pos = 1;

int yywrap(void);

#define ADJ \
do \
{ \
    EM_tokPos = yylval.pos = _char_pos; \
    _char_pos += yyleng; \
} \
while (0)

static int _comment_level = 0;

#define INIT_LEN 32
static string _str_buf;
static int _str_len;
static int _str_cap;
static void init_buf(void);
static void append_char(char ch);

%}

%x X_COMMENT X_STRING

%%

[ \t\f\v\r]             { ADJ; }
\n                      { ADJ; EM_newline(); }

"/*"                    { ADJ; _comment_level = 1; BEGIN(X_COMMENT); }
<X_COMMENT>[^*/\n]*       { ADJ; }
<X_COMMENT>"*"+[^*/\n]*   { ADJ; }
<X_COMMENT>"/"+[^*/\n]*   { ADJ; }
<X_COMMENT>\n             { ADJ; EM_newline(); }
<X_COMMENT>"/"+"*"        { ADJ; _comment_level++; }
<X_COMMENT>"*"+"/"        {
    ADJ;
    _comment_level--;
    if (_comment_level <= 0)
        BEGIN(INITIAL);
}

","                     { ADJ; return COMMA; }
":"                     { ADJ; return COLON; }
";"                     { ADJ; return SEMICOLON; }
"("                     { ADJ; return LPAREN; }
")"                     { ADJ; return RPAREN; }
"["                     { ADJ; return LBRACK; }
"]"                     { ADJ; return RBRACK; }
"{"                     { ADJ; return LBRACE; }
"}"                     { ADJ; return RBRACE; }
"."                     { ADJ; return DOT; }
"+"                     { ADJ; return PLUS; }
"-"                     { ADJ; return MINUS; }
"*"                     { ADJ; return TIMES; }
"/"                     { ADJ; return DIVIDE; }
"="                     { ADJ; return EQ; }
"<>"                    { ADJ; return NEQ; }
"<"                     { ADJ; return LT; }
"<="                    { ADJ; return LE; }
">"                     { ADJ; return GT; }
">="                    { ADJ; return GE; }
"&"                     { ADJ; return AND; }
"|"                     { ADJ; return OR; }
":="                    { ADJ; return ASSIGN; }

array                   { ADJ; return ARRAY; }
if                      { ADJ; return IF; }
then                    { ADJ; return THEN; }
else                    { ADJ; return ELSE; }
while                   { ADJ; return WHILE; }
for                     { ADJ; return FOR; }
to                      { ADJ; return TO; }
do                      { ADJ; return DO; }
let                     { ADJ; return LET; }
in                      { ADJ; return IN; }
end                     { ADJ; return END; }
of                      { ADJ; return OF; }
break                   { ADJ; return BREAK; }
nil                     { ADJ; return NIL; }
function                { ADJ; return FUNCTION; }
var                     { ADJ; return VAR; }
type                    { ADJ; return TYPE; }

[0-9]+                  { ADJ; yylval.ival = atoi(yytext); return INT; }
[_a-zA-Z][_a-zA-Z0-9]*  { ADJ; yylval.sval = String(yytext); return ID; }

\"                      { ADJ; init_buf(); BEGIN(X_STRING); }
<X_STRING>\"              {
    ADJ;
    BEGIN(INITIAL);
    yylval.sval = _str_buf;
    return STRING;
}
<X_STRING>\n              { ADJ; EM_newline(); }
<X_STRING>\\[0-9]{3}      {
    int result;

    ADJ;
    sscanf(yytext + 1, "%d", &result);
    if (result > 0xFF)
        EM_error(EM_tokPos, "character out of range");
    append_char(result);
}
<X_STRING>\\n             { ADJ; append_char('\n'); }
<X_STRING>\\t             { ADJ; append_char('\t'); }
<X_STRING>\\\"            { ADJ; append_char('\"'); }
<X_STRING>\\\\            { ADJ; append_char('\\'); }
<X_STRING>\\(.|\n)        { ADJ; append_char(yytext[1]); }
<X_STRING>[^\\\n\"]+      {
    char *p = yytext;

    ADJ;
    while (*p)
        append_char(*p++);
}

.                       { ADJ; EM_error(EM_tokPos, "illegal token"); }

%%

int yywrap(void) {
    _char_pos = 1;
    return 1;
}

static void init_buf(void) {
    _str_buf = checked_malloc(INIT_LEN);
    _str_buf[0] = 0;
    _str_len = 0;
    _str_cap = INIT_LEN;
}

static void append_char(char ch) {
    if (++_str_len == _str_cap) {
        char *p;

        _str_cap *= 2;
        p = checked_malloc(_str_cap);
        memcpy(p, _str_buf, _str_len);
        free(_str_buf);
        _str_buf = p;
    }
    _str_buf[_str_len - 1] = ch;
    _str_buf[_str_len] = 0;
}
