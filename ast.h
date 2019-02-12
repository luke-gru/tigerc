#ifndef tiger_ast_h
#define tiger_ast_h

#include "symbol.h"

/*
 * absyn.h - Abstract Syntax Header (Chapter 4)
 *
 * All types and functions declared in this header file begin with "N_" for
 * node. Linked list types end with "..list"
 */

/* Type Definitions */

typedef int Pos;

typedef struct sVar  *N_Var;
typedef struct sExpr *N_Expr;
typedef struct sDecl *N_Decl;
typedef struct sType *N_Type;

typedef struct sField     *N_Field;
typedef struct sFunDecl   *N_FunDecl;
typedef struct sNameType  *N_NameType;
typedef struct sEField    *N_EField;

typedef enum {PlusOp, MinusOp, TimesOp, DivideOp,
	     EqOp, NeqOp, LtOp, LeOp, GtOp, GeOp} Op;

struct sVar {
    enum {tSimpleVar, tFieldVar, tSubscriptVar} kind;
    Pos pos;
    union {
        Symbol simple;
        struct {
            N_Var var;
            Symbol sym;
        } field;
        struct {
            N_Var var;
            N_Expr expr;
        } subscript;
    } as;
};

struct sExpr {
    enum {tVarExpr, tNilExpr, tIntExpr, tStringExpr, tCallExpr,
        tOpExpr, tRecordExpr, tSeqExpr, tAssignExpr, tIfExpr,
        tWhileExpr, tForExpr, tBreakExpr, tLetExpr, tArrayExpr} kind;
       Pos pos;
       union {
           N_Var var;
           /* nil; - needs only the pos */
           int intVal;
           string stringVal;
           struct {Symbol func; List/*<N_Expr>*/ args;} call;
           struct {Op op; N_Expr left; N_Expr right;} op;
           struct {Symbol ty; List fields;} record;
           List/*<N_Expr>*/ seq;
           struct {N_Var var; N_Expr expr;} assign;
           struct {N_Expr test, then, elsee;} iff; /* elsee is optional */
           struct {N_Expr test, body;} whilee;
           struct {Symbol var; N_Expr lo,hi,body; bool escape;} forr;
           /* breakk; - need only the pos */
           struct {List/*<N_Decl>*/ decls; N_Expr body;} let;
           struct {Symbol ty; N_Expr size, init;} array;
	    } as;
};

struct sDecl {
    enum { tFunctionDecl, tVarDecl, tTypesDecl } kind;
    Pos pos;
    union {
        List/*<N_FunDecl>*/ functions;
        /* escape may change after the initial declaration */
        struct { Symbol var; Symbol ty; N_Expr init; bool escape;} var;
        List/*<N_NameType>*/ types;
    } as;
};

struct sType {
    enum { tNameTy, tRecordTy, tArrayTy } kind;
    Pos pos;
    union {
        Symbol name;
        List/*<N_Field>*/ record;
        Symbol array;
    } as;
};

/* Linked lists and nodes of lists */

struct sField { Symbol name, ty; Pos pos; bool escape; };

struct sFunDecl {
    Pos pos; Symbol name; List/*<N_Field>*/ params;
    Symbol result; N_Expr body;
};

struct sNameType { Pos pos; Symbol name; N_Type ty; };
struct sEField { Pos pos; Symbol name; N_Expr expr; };

/* Function prototypes for constructors */
N_Var  SimpleVar(Pos pos, Symbol sym);
N_Var  FieldVar(Pos pos, N_Var var, Symbol sym);
N_Var  SubscriptVar(Pos pos, N_Var var, N_Expr expr);
N_Expr VarExpr(Pos pos, N_Var var);
N_Expr NilExpr(Pos pos);
N_Expr IntExpr(Pos pos, int i);
N_Expr StringExpr(Pos pos, string s);
N_Expr CallExpr(Pos pos, Symbol func, List/*<N_Expr>*/ args);
N_Expr OpExpr(Pos pos, N_Expr left, Op op, N_Expr right);
N_Expr RecordExpr(Pos pos, Symbol ty, List/*<N_Field>*/ fields);
N_Expr SeqExpr(Pos pos, List/*<N_Expr>*/ seq);
N_Expr AssignExpr(Pos pos, N_Var var, N_Expr expr);
N_Expr IfExpr(Pos pos, N_Expr test, N_Expr then, N_Expr elsee);
N_Expr WhileExpr(Pos pos, N_Expr test, N_Expr body);
N_Expr ForExpr(Pos pos, Symbol var, N_Expr lo, N_Expr hi, N_Expr body);
N_Expr BreakExpr(Pos pos);
N_Expr LetExpr(Pos pos, List/*<N_Decl>*/ decls, N_Expr body);
N_Expr ArrayExpr(Pos pos, Symbol ty, N_Expr size, N_Expr init);
N_Decl FunctionDecl(Pos pos, List/*<N_FunDecl>*/ function);
N_Decl VarDecl(Pos pos, Symbol var, Symbol ty, N_Expr init);
N_Decl TypesDecl(Pos pos, List/*<N_NameType>*/ types);
N_Type NameType(Pos pos, Symbol name);
N_Type RecordType(Pos pos, List/*<N_Field>*/ record);
N_Type ArrayType(Pos pos, Symbol array);
N_Field Field(Pos pos, Symbol name, Symbol ty);
N_FunDecl FunDecl(Pos pos, Symbol name, List params/*<N_Field>*/, Symbol result,
                  N_Expr body);
N_NameType NamedType(Pos pos, Symbol name, N_Type ty);
N_EField EField(Pos pos, Symbol name, N_Expr exp);

#endif
