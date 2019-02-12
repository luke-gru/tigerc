%{
#include <stdio.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "ast.h"

int yylex(void); /* function prototype */

void yyerror(char *s) {
   EM_error(EM_tokPos, "%s", s);
}

static N_Expr _program;

%}

%union {
    int pos;
    int num;
    string str;
    List list;
    Symbol sym;
    N_Decl decl;
    N_Expr expr;
    N_Type type;
    N_Var var;
    N_FunDecl func;
}

%token <str> ID STRING
%token <num> INT

%token <pos>
  COMMA COLON SEMICOLON LPAREN RPAREN LBRACK RBRACK
  LBRACE RBRACE DOT
  PLUS MINUS TIMES DIVIDE EQ NEQ LT LE GT GE
  AND OR ASSIGN
  ARRAY IF THEN ELSE WHILE FOR TO DO LET IN END OF
  BREAK NIL
  FUNCTION VAR TYPE

%type <expr> program expr
%type <sym> id

%start program

%%

/* This is a skeleton grammar file, meant to illustrate what kind of
 * declarations are necessary above the %% mark.  Students are expected
 *  to replace the two dummy productions below with an actual grammar.
 */

program: expr { $$ = $1; _program = $1; }

expr: id

id: ID { $$ = GetSym($1); }
