%{

#define YY_NO_UNISTD_H

#include <ctype.h>
#include <string.h>

#include "util.h"
#include "errormsg.h"
#include "ast.h"
#include "parse.tab.h"

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

%option nounput
%option never-interactive

%x X_COMMENT X_STRING

%%

<INITIAL>[ \t\f\v\r]             { ADJ; }
<INITIAL>\n                      { ADJ; EM_newline(); }

<INITIAL>"/*"                    { ADJ; _comment_level = 1; BEGIN(X_COMMENT); }
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

<INITIAL>","                     { ADJ; return TK_COMMA; }
<INITIAL>":"                     { ADJ; return TK_COLON; }
<INITIAL>";"                     { ADJ; return TK_SEMICOLON; }
<INITIAL>"("                     { ADJ; return TK_LPAREN; }
<INITIAL>")"                     { ADJ; return TK_RPAREN; }
<INITIAL>"["                     { ADJ; return TK_LBRACK; }
<INITIAL>"]"                     { ADJ; return TK_RBRACK; }
<INITIAL>"{"                     { ADJ; return TK_LBRACE; }
<INITIAL>"}"                     { ADJ; return TK_RBRACE; }
<INITIAL>"."                     { ADJ; return TK_DOT; }
<INITIAL>"+"                     { ADJ; return TK_PLUS; }
<INITIAL>"-"                     { ADJ; return TK_MINUS; }
<INITIAL>"*"                     { ADJ; return TK_TIMES; }
<INITIAL>"/"                     { ADJ; return TK_DIVIDE; }
<INITIAL>"="                     { ADJ; return TK_EQ; }
<INITIAL>"<>"                    { ADJ; return TK_NEQ; }
<INITIAL>"<"                     { ADJ; return TK_LT; }
<INITIAL>"<="                    { ADJ; return TK_LE; }
<INITIAL>">"                     { ADJ; return TK_GT; }
<INITIAL>">="                    { ADJ; return TK_GE; }
<INITIAL>"&"                     { ADJ; return TK_AND; }
<INITIAL>"|"                     { ADJ; return TK_OR; }
<INITIAL>":="                    { ADJ; return TK_ASSIGN; }
<INITIAL>array                   { ADJ; return TK_ARRAY; }
<INITIAL>if                      { ADJ; return TK_IF; }
<INITIAL>then                    { ADJ; return TK_THEN; }
<INITIAL>else                    { ADJ; return TK_ELSE; }
<INITIAL>while                   { ADJ; return TK_WHILE; }
<INITIAL>for                     { ADJ; return TK_FOR; }
<INITIAL>to                      { ADJ; return TK_TO; }
<INITIAL>do                      { ADJ; return TK_DO; }
<INITIAL>let                     { ADJ; return TK_LET; }
<INITIAL>in                      { ADJ; return TK_IN; }
<INITIAL>end                     { ADJ; return TK_END; }
<INITIAL>of                      { ADJ; return TK_OF; }
<INITIAL>break                   { ADJ; return TK_BREAK; }
<INITIAL>nil                     { ADJ; return TK_NIL; }
<INITIAL>function                { ADJ; return TK_FUNCTION; }
<INITIAL>var                     { ADJ; return TK_VAR; }
<INITIAL>type                    { ADJ; return TK_TYPE; }
<INITIAL>[0-9]+                  { ADJ; yylval.num = atoi(yytext); return TK_INT; }
<INITIAL>[_a-zA-Z][_a-zA-Z0-9]*  { ADJ; yylval.str = String(yytext); return TK_ID; }
<INITIAL>\"                      { ADJ; init_buf(); BEGIN(X_STRING); }
<X_STRING>\"              {
    ADJ;
    BEGIN(INITIAL);
    yylval.str = _str_buf;
    return TK_STRING;
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

<INITIAL>.                       { ADJ; EM_error(EM_tokPos, "illegal token"); }

%%

// called at end of yyin's EOF
int yywrap(void) {
    _char_pos = 1;
    return 1; // signal this is last file
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
        assert(_str_buf);
        free(_str_buf);
        _str_buf = p;
    }
    _str_buf[_str_len - 1] = ch;
    _str_buf[_str_len] = 0;
}
