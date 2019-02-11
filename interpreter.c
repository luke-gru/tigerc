#include <stdio.h>
#include <string.h>
#include "interpreter.h"

Table MakeTable(string id, int value, Table tail) {
    Table t = CHECKED_MALLOC(struct sTable);
    t->id = id;
    t->value = value;
    t->tail = tail;
    return t;
}

IntAndTable MakeIntAndTable(int i, Table t) {
    IntAndTable val = CHECKED_MALLOC(struct sIntAndTable);
    val->i = i;
    val->t = t;
    return val;
}

int Lookup(Table t, string key) {
    Table cur = t;
    while (cur) {
        if (strcmp(key, cur->id) == 0) {
            return cur->value;
        }
        cur = cur->tail;
    }
    fprintf(stderr, "Key not found: %s\n", key);
    return -1;
}

Table Update(Table t, string key, int value) {
    return MakeTable(key, value, t);
}

static IntAndTable InterpretExpr(Expr e, Table t);
static Table InterpretCompoundStmt(Stmt s1, Stmt s2, Table t);
static Table InterpretAssignStmt(string id, Expr expr, Table t);
static Table InterpretPrintStmt(ExprList elist, Table t);

static IntAndTable InterpretOpExpr(Expr left, BinOp op, Expr right, Table t) {
    IntAndTable leftRes = InterpretExpr(left, t);
    IntAndTable rightRes = InterpretExpr(right, t);
    switch (op) {
        case OpPlus:
            return MakeIntAndTable(leftRes->i + rightRes->i, t);
        case OpMinus:
            return MakeIntAndTable(leftRes->i - rightRes->i, t);
        case OpTimes:
            return MakeIntAndTable(leftRes->i * rightRes->i, t);
        case OpDiv:
            assert(rightRes->i != 0);
            return MakeIntAndTable(leftRes->i / rightRes->i, t);
        default:
            assert(0);
    }
}

static Table InterpretStmt(Stmt s, Table t) {
    switch (s->kind) {
        case tCompoundStmt:
            return InterpretCompoundStmt(s->as.compound.stmt1, s->as.compound.stmt2, t);
        case tAssignStmt:
            return InterpretAssignStmt(s->as.assign.id, s->as.assign.expr, t);
        case tPrintStmt:
            return InterpretPrintStmt(s->as.print.exprs, t);
        default:
            assert(0);
    }
}

static IntAndTable InterpretExpr(Expr e, Table t) {
    switch (e->kind) {
        case tIdExpr:
            return MakeIntAndTable(Lookup(t, e->as.id), t);
        case tNumExpr:
            return MakeIntAndTable(e->as.num, t);
        case tOpExpr:
            return InterpretOpExpr(e->as.op.left, e->as.op.op, e->as.op.right, t);
        case tSeqExpr: {
            Table newT = InterpretStmt(e->as.seq.stmt, t);
            return InterpretExpr(e->as.seq.expr, newT);
        default:
            assert(0);
        }
    }
    return NULL;
}

static Table InterpretCompoundStmt(Stmt s1, Stmt s2, Table t) {
    Table newT = InterpretStmt(s1, t);
    return InterpretStmt(s2, newT);
}

static Table InterpretAssignStmt(string id, Expr expr, Table t) {
    IntAndTable res = InterpretExpr(expr, t);
    Table newT = Update(res->t, id, res->i);
    return newT;
}

static Table InterpretPrintStmt(ExprList elist, Table t) {
    Expr expr = NULL;
    FOREACH_EXPRLIST(elist, expr, {
        assert(expr);
        IntAndTable res = InterpretExpr(expr, t);
        t = res->t;
        fprintf(stdout, "%d\n", res->i);
    })
    return t;
}

Table Interpret(Stmt program, Table env) {
    if (env == NULL) {
        env = MakeTable("", 0, NULL);
    }
    return InterpretStmt(program, env);
}
