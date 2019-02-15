#include <stdio.h>
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "table.h"

static Symbol MakeSym(string name, Symbol next) {
    Symbol s = CHECKED_MALLOC(struct sSymbol);
    s->name = name;
    s->next = next;
    return s;
}

#define SIZE 109  /* should be prime */

static Symbol hashtable[SIZE];

static unsigned int hash(char *s0) {
    unsigned int h = 0;
    char *s;
    for (s = s0; *s; s++) {
        h = h * 65599 + *s;
    }
    return h;
}

static int streq(string a, string b) {
 return !strcmp(a,b);
}

Symbol GetSym(string name) {
    int index= hash(name) % SIZE;
    Symbol syms = hashtable[index], sym;
    for (sym=syms; sym; sym=sym->next) {
        if (streq(sym->name, name)) return sym;
    }
    // not found,
    sym = MakeSym(name,syms);
    hashtable[index] = sym;
    return sym;
}

string SymName(Symbol sym) {
    return sym->name;
}

bool SymEq(Symbol s1, Symbol s2) {
    return strcmp(SymName(s1), SymName(s2)) == 0;
}

SymTable MakeSymTable(void) {
    return MakeTable();
}

void SymTableEnter(SymTable st, Symbol sym, void *value) {
    TableEnter(st, sym, value);
}

void *SymTableLookup(SymTable st, Symbol sym) {
  return TableLookup(st, sym);
}

// scope boundary sentinel symbol
static struct sSymbol marksym = { "<mark>", 0 };

void SymTableBeginScope(SymTable st) {
    SymTableEnter(st, &marksym, NULL);
}

void SymTableEndScope(SymTable st) {
    Symbol s;
    do {
        s = TablePop(st);
    } while (s != &marksym);
}

/*void S_dump(S_table t, void (*show)(S_symbol sym, void *binding)) {*/
  /*TAB_dump(t, (void (*)(void *, void *)) show);*/
/*}*/
