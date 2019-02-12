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

typedef struct sDeclList   *N_DeclList;
typedef struct sExprList  *N_ExprList;
typedef struct sField     *N_Field;
typedef struct sFieldList *N_FieldList;
typedef struct sFunDecl   *N_FunDecl;
typedef struct sFunDeclList *N_FunDeclList;
typedef struct sNameType    *N_NameType;
typedef struct sNameTypeList *N_NameTypeList;
typedef struct sEField       *N_EField;
typedef struct sEFieldList   *N_EFieldList;

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
           struct {Symbol func; N_ExprList args;} call;
           struct {Op op; N_Expr left; N_Expr right;} op;
           struct {Symbol ty; N_EFieldList fields;} record;
           N_ExprList seq;
           struct {N_Var var; N_Expr expr;} assign;
           struct {N_Expr test, then, elsee;} iff; /* elsee is optional */
           struct {N_Expr test, body;} whilee;
           struct {Symbol var; N_Expr lo,hi,body; bool escape;} forr;
           /* breakk; - need only the pos */
           struct {N_DeclList decls; N_Expr body;} let;
           struct {Symbol ty; N_Expr size, init;} array;
	    } as;
};

struct sDecl {
    enum { tFunctionDecl, tVarDecl, tTypeDecl } kind;
    Pos pos;
    union {
        N_FunDeclList function;
        /* escape may change after the initial declaration */
        struct { Symbol var; Symbol ty; N_Expr init; bool escape;} var;
        N_NameTypeList ty;
    } as;
};

struct sType {
    enum { tNameTy, tRecordTy, tArrayTy } kind;
    Pos pos;
    union {
        Symbol name;
        N_FieldList record;
        Symbol array;
    } as;
};

/* Linked lists and nodes of lists */

struct sField { Symbol name, ty; Pos pos; bool escape; };
struct sFieldList { N_Field head; N_FieldList tail; };

struct sExprList { N_Expr head; N_ExprList tail; };
struct sFunDecl {
    Pos pos; Symbol name; N_FieldList params;
    Symbol result; N_Expr body;
};

struct sFunDeclList { N_FunDecl head; N_FunDeclList tail; };
struct sDeclList { N_Decl head; N_DeclList tail; };

struct sNameType { Symbol name; N_Type ty; };
struct sNameTypeList { N_NameType head; N_NameTypeList tail; };
struct sEField { Symbol name; N_Expr expr; };
struct sEFieldList { N_EField head; N_EFieldList tail; };

/* Function prototypes for constructors */
N_Var  SimpleVar(Pos pos, Symbol sym);
N_Var  FieldVar(Pos pos, N_Var var, Symbol sym);
N_Var  SubscriptVar(Pos pos, N_Var var, N_Expr expr);
N_Expr VarExpr(Pos pos, N_Var var);
N_Expr NilExpr(Pos pos);
N_Expr IntExpr(Pos pos, int i);
N_Expr StringExpr(Pos pos, string s);
N_Expr CallExpr(Pos pos, Symbol func, N_ExprList args);
N_Expr OpExpr(Pos pos, Op op, N_Expr left, N_Expr right);
N_Expr RecordExpr(Pos pos, Symbol ty, N_EFieldList fields);
N_Expr SeqExpr(Pos pos, N_ExprList seq);
N_Expr AssignExpr(Pos pos, N_Var var, N_Expr expr);
N_Expr IfExpr(Pos pos, N_Expr test, N_Expr then, N_Expr elsee);
N_Expr WhileExpr(Pos pos, N_Expr test, N_Expr body);
N_Expr ForExpr(Pos pos, Symbol var, N_Expr lo, N_Expr hi, N_Expr body);
N_Expr BreakExpr(Pos pos);
N_Expr LetExpr(Pos pos, N_DeclList decls, N_Expr body);
N_Expr ArrayExpr(Pos pos, Symbol ty, N_Expr size, N_Expr init);
N_Decl FunctionDecl(Pos pos, N_FunDeclList function);
N_Decl VarDecl(Pos pos, Symbol var, Symbol ty, N_Expr init);
N_Decl TypeDecl(Pos pos, N_NameTypeList type);
N_Type NameType(Pos pos, Symbol name);
N_Type RecordType(Pos pos, N_FieldList record);
N_Type ArrayType(Pos pos, Symbol array);
N_Field Field(Pos pos, Symbol name, Symbol ty);
N_FieldList FieldList(N_Field head, N_FieldList tail);
N_ExprList ExprList(N_Expr head, N_ExprList tail);
N_FunDecl FunDecl(Pos pos, Symbol name, N_FieldList params, Symbol result,
                  N_Expr body);
N_FunDeclList FunDeclList(N_FunDecl head, N_FunDeclList tail);
N_DeclList DeclList(N_Decl head, N_DeclList tail);
N_NameType NamedType(Symbol name, N_Type ty);
N_NameTypeList NameTypeList(N_NameType head, N_NameTypeList tail);
N_EField EField(Symbol name, N_Expr exp);
N_EFieldList EFieldList(N_EField head, N_EFieldList tail);

#endif
