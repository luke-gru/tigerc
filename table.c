/*
 * table.c - Functions to manipulate generic tables.
 * Copyright (c) 1997 Andrew W. Appel.
 */

#include <stdio.h>
#include <inttypes.h>
#include "util.h"
#include "table.h"

#define TABSIZE 127

typedef struct sBinder *Binder;

struct sBinder {void *key; void *value; Binder next; void *prevtop;};

struct sTable {
  Binder table[TABSIZE];
  void *top; // for pop()
};


static Binder MakeBinder(void *key, void *value, Binder next, void *prevtop) {
    Binder b = CHECKED_MALLOC(struct sBinder);
    b->key = key;
    b->value = value;
    b->next = next;
    b->prevtop = prevtop;
    return b;
}

Table MakeTable(void) {
    Table t = CHECKED_MALLOC(struct sTable);
    t->top = NULL;
    for (int i = 0; i < TABSIZE; i++) {
        t->table[i] = NULL;
    }
    return t;
}

/* The cast from pointer to integer in the expression
 *   ((unsigned)key) % TABSIZE
 * may lead to a warning message.  However, the code is safe,
 * and will still operate correctly.  This line is just hashing
 * a pointer value into an integer value, and no matter how the
 * conversion is done, as long as it is done consistently, a
 * reasonable and repeatable index into the table will result.
 */

void TableEnter(Table t, void *key, void *value) {
    int index;
    assert(t && key);
    index = ((size_t)key) % TABSIZE;
    t->table[index] = MakeBinder(key, value, t->table[index], t->top);
    t->top = key;
}

void *TableLookup(Table t, void *key) {
    int index;
    Binder b;
    assert(t && key);
    index=((size_t)key) % TABSIZE;
    for (b = t->table[index]; b; b = b->next) {
        if (b->key==key) return b->value;
    }
    return NULL;
}

void *TablePop(Table t) {
    void *k; Binder b; int index;
    assert(t);
    k = t->top;
    assert(k);
    index = ((size_t)k) % TABSIZE;
    b = t->table[index];
    assert(b);
    t->table[index] = b->next;
    t->top = b->prevtop;
    return b->key;
}

void TableDump(Table t, void (*show)(void *key, void *value)) {
    void *k = t->top;
    int index = ((size_t)k) % TABSIZE;
    Binder b = t->table[index];
    if (b==NULL) return; // base case, no more
    t->table[index]=b->next;
    t->top = b->prevtop;
    show(b->key, b->value);
    TableDump(t, show); // recursively dump next key-value pairs
    assert(t->top == b->prevtop && t->table[index] == b->next);
    t->top = k;
    t->table[index] = b;
}
