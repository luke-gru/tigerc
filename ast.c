/*
 * absyn.c - Abstract Syntax Functions. Most functions create an instance of an
 *           abstract syntax rule.
 */

#include "util.h"
#include "symbol.h" /* symbol table data structures */
#include "ast.h"  /* abstract syntax data structures */

N_Var SimpleVar(Pos pos, Symbol sym) {
    N_Var p = CHECKED_MALLOC(struct sVar);
    p->kind = tSimpleVar;
    p->pos = pos;
    p->as.simple = sym;
    return p;
}

N_Var FieldVar(Pos pos, N_Var var, Symbol sym) {
    N_Var p = CHECKED_MALLOC(struct sVar);
    p->kind = tFieldVar;
    p->pos = pos;
    p->as.field.var = var;
    p->as.field.sym = sym;
    return p;
}

N_Var SubscriptVar(Pos pos, N_Var var, N_Expr expr) {
    N_Var p = CHECKED_MALLOC(struct sVar);
    p->kind = tSubscriptVar;
    p->pos = pos;
    p->as.subscript.var = var;
    p->as.subscript.expr = expr;
    return p;
}

N_Expr VarExpr(Pos pos, N_Var var) {
    N_Expr p = CHECKED_MALLOC(struct sExpr);
    p->kind = tVarExpr;
    p->pos = pos;
    p->as.var = var;
    return p;
}

N_Expr NilExpr(Pos pos) {
    N_Expr p = CHECKED_MALLOC(struct sExpr);
    p->kind = tNilExpr;
    p->pos = pos;
    return p;
}

N_Expr IntExpr(Pos pos, int i) {
    N_Expr p = CHECKED_MALLOC(struct sExpr);
    p->kind = tIntExpr;
    p->pos=pos;
    p->as.intVal=i;
    return p;
}

N_Expr StringExpr(Pos pos, string s) {
    N_Expr p = CHECKED_MALLOC(struct sExpr);
    p->kind=tStringExpr;
    p->pos=pos;
    p->as.stringVal=s;
    return p;
}

N_Expr CallExpr(Pos pos, Symbol func, List args) {
    N_Expr p = CHECKED_MALLOC(struct sExpr);
    p->kind=tCallExpr;
    p->pos=pos;
    p->as.call.func=func;
    p->as.call.args=args;
    return p;
}

N_Expr OpExpr(Pos pos, N_Expr left, Op op, N_Expr right) {
    N_Expr p = CHECKED_MALLOC(struct sExpr);
    p->kind=tOpExpr;
    p->pos=pos;
    p->as.op.op=op;
    p->as.op.left=left;
    p->as.op.right=right;
    return p;
}

N_Expr RecordExpr(Pos pos, Symbol ty, List fields) {
    N_Expr p = CHECKED_MALLOC(struct sExpr);
    p->kind=tRecordExpr;
    p->pos=pos;
    p->as.record.ty=ty;
    p->as.record.efields=fields;
    return p;
}

N_Expr SeqExpr(Pos pos, List seq) {
    N_Expr p = CHECKED_MALLOC(struct sExpr);
    p->kind=tSeqExpr;
    p->pos=pos;
    p->as.seq=seq;
    return p;
}

N_Expr AssignExpr(Pos pos, N_Var var, N_Expr expr) {
    N_Expr p = CHECKED_MALLOC(struct sExpr);
    p->kind=tAssignExpr;
    p->pos=pos;
    p->as.assign.var=var;
    p->as.assign.expr=expr;
    return p;
}

N_Expr IfExpr(Pos pos, N_Expr test, N_Expr then, N_Expr elsee) {
    N_Expr p = CHECKED_MALLOC(struct sExpr);
    p->kind=tIfExpr;
    p->pos=pos;
    p->as.iff.test=test;
    p->as.iff.then=then;
    p->as.iff.elsee=elsee;
    return p;
}

N_Expr WhileExpr(Pos pos, N_Expr test, N_Expr body) {
    N_Expr p = CHECKED_MALLOC(struct sExpr);
    p->kind=tWhileExpr;
    p->pos=pos;
    p->as.whilee.test=test;
    p->as.whilee.body=body;
    return p;
}

N_Expr ForExpr(Pos pos, Symbol var, N_Expr lo, N_Expr hi, N_Expr body) {
    N_Expr p = CHECKED_MALLOC(struct sExpr);
    p->kind=tForExpr;
    p->pos=pos;
    p->as.forr.var=var;
    p->as.forr.lo=lo;
    p->as.forr.hi=hi;
    p->as.forr.body=body;
    p->as.forr.escape=true;
    return p;
}

N_Expr BreakExpr(Pos pos) {
    N_Expr p = CHECKED_MALLOC(struct sExpr);
    p->kind=tBreakExpr;
    p->pos=pos;
    return p;
}

N_Expr LetExpr(Pos pos, List decls, N_Expr body) {
    N_Expr p = CHECKED_MALLOC(struct sExpr);
    p->kind=tLetExpr;
    p->pos=pos;
    p->as.let.decls=decls;
    p->as.let.body=body;
    return p;
}

N_Expr ArrayExpr(Pos pos, Symbol ty, N_Expr size, N_Expr init) {
    N_Expr p = CHECKED_MALLOC(struct sExpr);
    p->kind=tArrayExpr;
    p->pos=pos;
    p->as.array.ty=ty;
    p->as.array.size=size;
    p->as.array.init=init;
    return p;
}

N_Decl FunctionDecl(Pos pos, List functions) {
    N_Decl p = CHECKED_MALLOC(struct sDecl);
    p->kind=tFunctionDecl;
    p->pos=pos;
    p->as.functions=functions;
    return p;
}

N_Decl VarDecl(Pos pos, Symbol var, Symbol ty, N_Expr init) {
    N_Decl p = CHECKED_MALLOC(struct sDecl);
    p->kind=tVarDecl;
    p->pos=pos;
    p->as.var.var=var;
    p->as.var.ty=ty;
    p->as.var.init=init;
    p->as.var.escape=true;
    return p;
}

N_Decl TypesDecl(Pos pos, List types) {
    N_Decl p = CHECKED_MALLOC(struct sDecl);
    p->kind=tTypesDecl;
    p->pos=pos;
    p->as.types=types;
    return p;
}

N_Type NameType(Pos pos, Symbol name) {
    N_Type p = CHECKED_MALLOC(struct sType);
    p->kind=tNameTy;
    p->pos=pos;
    p->as.name=name;
    return p;
}

N_Type RecordType(Pos pos, List record) {
    N_Type p = CHECKED_MALLOC(struct sType);
    p->kind=tRecordTy;
    p->pos=pos;
    p->as.record=record;
    return p;
}

N_Type ArrayType(Pos pos, Symbol array) {
    N_Type p = CHECKED_MALLOC(struct sType);
    p->kind=tArrayTy;
    p->pos=pos;
    p->as.array=array;
    return p;
}

N_Field Field(Pos pos, Symbol name, Symbol ty) {
    N_Field p = CHECKED_MALLOC(struct sField);
    p->pos=pos;
    p->name=name;
    p->ty=ty;
    p->escape=true;
    return p;
}

N_FunDecl FunDecl(Pos pos, Symbol name, List params, Symbol result,
		  N_Expr body) {
    N_FunDecl p = CHECKED_MALLOC(struct sFunDecl);
    p->pos=pos;
    p->name=name;
    p->params=params;
    p->result=result;
    p->body=body;
    return p;
}

N_NameType NamedType(Pos pos, Symbol name, N_Type ty) {
    N_NameType p = CHECKED_MALLOC(struct sNameType);
    p->pos = pos;
    p->name=name;
    p->ty=ty;
    return p;
}

N_EField EField(Pos pos, Symbol name, N_Expr expr) {
    N_EField p = CHECKED_MALLOC(struct sEField);
    p->pos = pos;
    p->name=name;
    p->expr=expr;
    return p;
}

