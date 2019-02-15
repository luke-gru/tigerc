#ifndef tiger_symbol_h
#define tiger_symbol_h

#include "util.h"

/*
 * symbol.h - Symbols and symbol tables
 *
 */

struct sSymbol {
    string name;
    struct sSymbol *next;
};

typedef struct sSymbol *Symbol;

/* Make a unique symbol from a given string.
 *  Different calls to GetSym("foo") will yield the same Symbol
 *  value, even if the "foo" strings are at different locations. */
Symbol GetSym(string);

/* Extract the underlying string from a symbol */
string SymName(Symbol);

bool SymEq(Symbol s1, Symbol s2);

/** SymTable is a mapping from Symbol->any, where "any" is represented
 * here by `void*`. `void*` holds either type information, or value (function, variable)
 * info
 */
typedef struct sTable *SymTable;

/* Make a new table */
SymTable MakeSymTable(void);

/* Enter a binding "sym->value" into "t", shadowing but not deleting
 *    any previous binding of "sym". */
void SymTableEnter(SymTable st, Symbol sym, void *value);

/* Look up the most recent binding of "sym" in "t", or return NULL
 *    if sym is unbound. */
void *SymTableLookup(SymTable st, Symbol sym);

/* Start a new "scope" in "st".  Scopes are nested. */
void SymTableBeginScope(SymTable st);

/* Remove any bindings entered since the current scope began,
   and end the current scope. */
void SymTableEndScope(SymTable st);

#endif
