#ifndef tiger_interpreter_h
#define tiger_interpreter_h

#include "nodes.h"
#include "util.h"

struct sTable;
struct sIntAndTable;
typedef struct sTable *Table;
typedef struct sIntAndTable *IntAndTable;

struct sTable {
    string id;
    int value;
    Table tail;
};

// interpreter values
struct sIntAndTable {
    int i;
    Table t;
};

Table MakeTable(string id, int value, Table tail);
IntAndTable MakeIntAndTable(int value, Table t);
int Lookup(Table t, string key);
Table Update(Table t, string key, int value);

Table Interpret(Stmt program, Table env);

#endif
