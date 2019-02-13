%{
#include <stdio.h>
#include <string.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "ast.h"

#define YYDEBUG 1

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

%token <str> TK_ID TK_STRING
%token <num> TK_INT

%token <pos>
  TK_COMMA TK_COLON TK_LPAREN TK_RPAREN TK_LBRACK TK_RBRACK
  TK_LBRACE TK_RBRACE TK_DOT
  TK_ARRAY TK_IF TK_THEN TK_ELSE TK_WHILE TK_FOR TK_TO TK_LET TK_IN TK_END TK_OF
  TK_BREAK TK_NIL
  TK_FUNCTION TK_VAR TK_TYPE

%left <pos> TK_SEMICOLON
%nonassoc <pos> TK_DO
%nonassoc <pos>  TK_ASSIGN
%left <pos>  TK_OR
%left <pos>  TK_AND
%nonassoc <pos>  TK_EQ TK_NEQ TK_LT TK_LE TK_GT TK_GE
%left <pos>  TK_PLUS TK_MINUS
%left <pos>  TK_TIMES TK_DIVIDE
%left <pos>  TK_UMINUS

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

program:
    expr
    { _program = $1; }

expr:
    lvalue
    { $$ = VarExpr($1->pos, $1); }
|   TK_NIL
    { $$ = NilExpr($1); }
|   expr expr_seq
    { $$ = SeqExpr($1->pos, DataList($1, $2)); }
|   TK_LPAREN TK_RPAREN
    { $$ = SeqExpr($1, NULL); }
|   TK_LPAREN expr TK_RPAREN
    { $$ = $2; }
|   TK_INT
    { $$ = IntExpr(EM_tokPos, $1); }
|   TK_STRING
    { $$ = StringExpr(EM_tokPos, $1); }
|   TK_MINUS expr %prec TK_UMINUS
    { $$ = OpExpr($1, $2, MinusOp, NULL); }

|   id TK_LPAREN TK_RPAREN
    { $$ = CallExpr($2, $1, NULL); }
|   id TK_LPAREN expr arg_seq TK_RPAREN
    { $$ = CallExpr($2, $1, DataList($3, $4)); }

|   expr TK_PLUS expr
    { $$ = OpExpr($2, $1, PlusOp, $3); }
|   expr TK_MINUS expr
    { $$ = OpExpr($2, $1, MinusOp, $3); }
|   expr TK_TIMES expr
    { $$ = OpExpr($2, $1, TimesOp, $3); }
|   expr TK_DIVIDE expr
    { $$ = OpExpr($2, $1, DivideOp, $3); }
|   expr TK_EQ expr
    { $$ = OpExpr($2, $1, EqOp, $3); }
|   expr TK_NEQ expr
    { $$ = OpExpr($2, $1, NeqOp, $3); }
|   expr TK_LT expr
    { $$ = OpExpr($2, $1, LtOp, $3); }
|   expr TK_LE expr
    { $$ = OpExpr($2, $1, LeOp, $3); }
|   expr TK_GT expr
    { $$ = OpExpr($2, $1, GtOp, $3); }
|   expr TK_GE expr
    { $$ = OpExpr($2, $1, GeOp, $3); }

|   expr TK_AND expr
    { $$ = IfExpr($2, $1, $3, IntExpr($2, 0)); }
|   expr TK_OR expr
    { $$ = IfExpr($2, $1, IntExpr($2, 1), $3); }

|   id TK_LBRACE TK_RBRACE
    { $$ = RecordExpr($2, $1, NULL); }
|   id TK_LBRACE id TK_EQ expr efield_seq TK_RBRACE
    { $$ = RecordExpr($2, $1, DataList(EField($4, $3, $5), $6)); }

|   id TK_LBRACK expr TK_RBRACK TK_OF expr
    { $$ = ArrayExpr($2, $1, $3, $6); }

|   lvalue TK_ASSIGN expr
    { $$ = AssignExpr($2, $1, $3); }

|   TK_IF expr TK_THEN expr
    { $$ = IfExpr($1, $2, $4, NULL); }
|   TK_IF expr TK_THEN expr TK_ELSE expr
    { $$ = IfExpr($1, $2, $4, $6); }

|   TK_WHILE expr TK_DO expr
    { $$ = WhileExpr($1, $2, $4); }
|   TK_FOR id TK_ASSIGN expr TK_TO expr TK_DO expr
    { $$ = ForExpr($1, $2, $4, $6, $8); }
|   TK_BREAK
    { $$ = BreakExpr($1); }

|   TK_LET decls TK_IN expr TK_END
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
    TK_TYPE id TK_EQ type
    { $$ = DataList(NamedType($1, $2, $4), NULL); }
|   types_decl TK_TYPE id TK_EQ type
    { LIST_ACTION($$, $1, NamedType($2, $3, $5)); }

type:
    id
    { $$ = NameType(EM_tokPos, $1); }
|   TK_LBRACE fields TK_RBRACE
    { $$ = RecordType($1, $2); }
|   TK_ARRAY TK_OF id
    { $$ = ArrayType($1, $3); }

fields:
    /* empty */
    { $$ = NULL; }
|   id TK_COLON id field_seq
    { $$ = DataList(Field($2, $1, $3), $4); }

var_decl:
    TK_VAR id TK_ASSIGN expr
    { $$ = VarDecl($1, $2, NULL, $4); }
|   TK_VAR id TK_COLON id TK_ASSIGN expr
    { $$ = VarDecl($1, $2, $4, $6); }

funcs_decl:
    func_decl
    { $$ = DataList($1, NULL); }
|   funcs_decl func_decl
    { LIST_ACTION($$, $1, $2); }

func_decl:
    TK_FUNCTION id TK_LPAREN fields TK_RPAREN TK_EQ expr
    { $$ = FunDecl($1, $2, $4, NULL, $7); }
|   TK_FUNCTION id TK_LPAREN fields TK_RPAREN TK_COLON id TK_EQ expr
    { $$ = FunDecl($1, $2, $4, $7, $9); }

expr_seq:
    TK_SEMICOLON expr
    { $$ = DataList($2, NULL); }
|   expr_seq TK_SEMICOLON expr
    { LIST_ACTION($$, $1, $3); }

arg_seq:
    /* empty */
    { $$ = NULL; }
|   arg_seq TK_COMMA expr
    { LIST_ACTION($$, $1, $3); }

efield_seq:
    /* empty */
    { $$ = NULL; }
|   efield_seq TK_COMMA id TK_EQ expr
    { LIST_ACTION($$, $1, EField($4, $3, $5)); }

field_seq:
    /* empty */
    { $$ = NULL; }
|   field_seq TK_COMMA id TK_COLON id
    { LIST_ACTION($$, $1, Field($4, $3, $5)); }

lvalue:
    id lvalue_
    { LVALUE_ACTION($$, $2, SimpleVar(EM_tokPos, $1)); }

lvalue_:
    /* empty */
    { $$ = NULL; }
|   TK_DOT id lvalue_
    { LVALUE_ACTION($$, $3, FieldVar($1, NULL, $2)); }
|   TK_LBRACK expr TK_RBRACK lvalue_
    { LVALUE_ACTION($$, $4, SubscriptVar($1, NULL, $2)); }

id:
    TK_ID
    { $$ = GetSym($1); }

%%

void yyerror(char *msg) {
    EM_error(EM_tokPos, "%s", msg);
}

static void print_token_value(FILE *fp, int type, YYSTYPE value) {
    switch (type) {
        case TK_ID:
        case TK_STRING:
            fprintf(fp, "%s", value.str);
            break;
        case TK_INT:
            fprintf(fp, "%d", value.num);
            break;
        default:
            fprintf(fp, "tok %d", type);
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
