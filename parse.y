%{
#include <stdio.h>
#include <string.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "ast.h"

int yylex(void);
void yyerror(char *msg);

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

%{

static void print_token_value(FILE *fp, int type, YYSTYPE value);
#define YYPRINT(fp, type, value) print_token_value(fp, type, value)

// add elem to end of list, assign $$ (target) to list
#define LIST_ACTION(target, prev, elem) \
    do \
    { \
        List p, e = DataList((elem), NULL); \
        (target) = p = (prev); \
        if (p) \
        { \
            while (p->next) \
                p = p->next; \
            p->next = e; \
        } \
        else \
            (target) = e; \
    } \
    while (false)

#define LVALUE_ACTION(target, prev, elem) \
    do \
    { \
        N_Var p, var = (elem); \
        (target) = p = (prev); \
        if (p) \
        { \
            while (p->as.field.var) \
                p = p->as.field.var; \
            p->as.field.var = var; \
        } \
        else \
            (target) = var; \
    } \
    while (false)
%}

%debug

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

%left SEMICOLON
%nonassoc DO
%nonassoc  ASSIGN
%left  OR
%left  AND
%nonassoc  EQ NEQ LT LE GT GE
%left  PLUS MINUS
%left  TIMES DIVIDE
%left  UMINUS

%type <decl> decl var_decl
%type <expr> program expr
%type <type> type
%type <var> lvalue lvalue_
%type <list> expr_seq arg_seq efield_seq decls funcs_decl types_decl fields
%type <list> field_seq
%type <func> func_decl
%type <sym> id

%start program

%%

program: expr { _program = $1; }

expr:
    lvalue
    { $$ = VarExpr($1->pos, $1); }
|   NIL
    { $$ = NilExpr($1); }
|   expr expr_seq
    { $$ = SeqExpr($1->pos, DataList($1, $2)); }
|   LPAREN RPAREN
    { $$ = SeqExpr($1, NULL); }
|   LPAREN expr RPAREN
    { $$ = $2; }
|   INT
    { $$ = IntExpr(EM_tokPos, $1); }
|   STRING
    { $$ = StringExpr(EM_tokPos, $1); }
|   MINUS expr %prec UMINUS
    { $$ = OpExpr($1, $2, MinusOp, NULL); }

|   id LPAREN RPAREN
    { $$ = CallExpr($2, $1, NULL); }
|   id LPAREN expr arg_seq RPAREN
    { $$ = CallExpr($2, $1, DataList($3, $4)); }

|   expr PLUS expr
    { $$ = OpExpr($2, $1, PlusOp, $3); }
|   expr MINUS expr
    { $$ = OpExpr($2, $1, MinusOp, $3); }
|   expr TIMES expr
    { $$ = OpExpr($2, $1, TimesOp, $3); }
|   expr DIVIDE expr
    { $$ = OpExpr($2, $1, DivideOp, $3); }
|   expr EQ expr
    { $$ = OpExpr($2, $1, EqOp, $3); }
|   expr NEQ expr
    { $$ = OpExpr($2, $1, NeqOp, $3); }
|   expr LT expr
    { $$ = OpExpr($2, $1, LtOp, $3); }
|   expr LE expr
    { $$ = OpExpr($2, $1, LeOp, $3); }
|   expr GT expr
    { $$ = OpExpr($2, $1, GtOp, $3); }
|   expr GE expr
    { $$ = OpExpr($2, $1, GeOp, $3); }

|   expr AND expr
    { $$ = IfExpr($2, $1, $3, IntExpr($2, 0)); }
|   expr OR expr
    { $$ = IfExpr($2, $1, IntExpr($2, 1), $3); }

|   id LBRACE RBRACE
    { $$ = RecordExpr($2, $1, NULL); }
|   id LBRACE id EQ expr efield_seq RBRACE
    { $$ = RecordExpr($2, $1, DataList(EField($4, $3, $5), $6)); }

|   id LBRACK expr RBRACK OF expr
    { $$ = ArrayExpr($2, $1, $3, $6); }

|   lvalue ASSIGN expr
    { $$ = AssignExpr($2, $1, $3); }

|   IF expr THEN expr
    { $$ = IfExpr($1, $2, $4, NULL); }
|   IF expr THEN expr ELSE expr
    { $$ = IfExpr($1, $2, $4, $6); }

|   WHILE expr DO expr
    { $$ = WhileExpr($1, $2, $4); }
|   FOR id ASSIGN expr TO expr DO expr
    { $$ = ForExpr($1, $2, $4, $6, $8); }
|   BREAK
    { $$ = BreakExpr($1); }

|   LET decls IN expr END
    { $$ = LetExpr($1, $2, $4); }

decls:
    /* empty */
    { $$ = NULL; }
|   decls decl
    { LIST_ACTION($$, $1, $2); }

decl:
    types_decl
    { $$ = TypesDecl(((N_NameType)$1->data)->pos, $1); }
|   var_decl
|   funcs_decl
    { $$ = FunctionDecl(((N_FunDecl) $1->data)->pos, $1); }

types_decl:
    TYPE id EQ type
    { $$ = DataList(NamedType($1, $2, $4), NULL); }
|   types_decl TYPE id EQ type
    { LIST_ACTION($$, $1, NamedType($2, $3, $5)); }

type:
    id
    { $$ = NameType(EM_tokPos, $1); }
|   LBRACE fields RBRACE
    { $$ = RecordType($1, $2); }
|   ARRAY OF id
    { $$ = ArrayType($1, $3); }

fields:
    /* empty */
    { $$ = NULL; }
|   id COLON id field_seq
    { $$ = DataList(Field($2, $1, $3), $4); }

var_decl:
    VAR id ASSIGN expr
    { $$ = VarDecl($1, $2, NULL, $4); }
|   VAR id COLON id ASSIGN expr
    { $$ = VarDecl($1, $2, $4, $6); }

funcs_decl:
    func_decl
    { $$ = DataList($1, NULL); }
|   funcs_decl func_decl
    { LIST_ACTION($$, $1, $2); }

func_decl:
    FUNCTION id LPAREN fields RPAREN EQ expr
    { $$ = FunDecl($1, $2, $4, NULL, $7); }
|   FUNCTION id LPAREN fields RPAREN COLON id EQ expr
    { $$ = FunDecl($1, $2, $4, $7, $9); }

expr_seq:
    SEMICOLON expr
    { $$ = DataList($2, NULL); }
|   expr_seq SEMICOLON expr
    { LIST_ACTION($$, $1, $3); }

arg_seq:
    /* empty */
    { $$ = NULL; }
|   arg_seq COMMA expr
    { LIST_ACTION($$, $1, $3); }

efield_seq:
    /* empty */
    { $$ = NULL; }
|   efield_seq COMMA id EQ expr
    { LIST_ACTION($$, $1, EField($4, $3, $5)); }

field_seq:
    /* empty */
    { $$ = NULL; }
|   field_seq COMMA id COLON id
    { LIST_ACTION($$, $1, Field($4, $3, $5)); }

lvalue:
    id lvalue_
    { LVALUE_ACTION($$, $2, SimpleVar(EM_tokPos, $1)); }

lvalue_:
    /* empty */
    { $$ = NULL; }
|   DOT id lvalue_
    { LVALUE_ACTION($$, $3, FieldVar($1, NULL, $2)); }
|   LBRACK expr RBRACK lvalue_
    { LVALUE_ACTION($$, $4, SubscriptVar($1, NULL, $2)); }

id:
    ID { $$ = GetSym($1); }
%%

void yyerror(char *msg) {
    EM_error(EM_tokPos, "%s", msg);
}

static void print_token_value(FILE *fp, int type, YYSTYPE value) {
    switch (type) {
        case ID:
        case STRING:
            fprintf(fp, "%s", value.str);
            break;
        case INT:
            fprintf(fp, "%d", value.num);
            break;
    }
}

N_Expr parse(string filename) {
    if (strcmp(filename, "-") == 0) {
        EM_fset(stdin, "stdin");
    } else {
        EM_reset(filename);
    }
    if (yyparse() == 0) {
        return _program;
    } else {
        return NULL;
    }
}
