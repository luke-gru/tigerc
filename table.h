#ifndef tiger_table_h
#define tiger_table_h

/*
 * table.h - generic hash table
 *
 * No algorithm should use these functions directly, because
 *  programming with void* is too error-prone.  Instead,
 *  each module should make "wrapper" functions that take
 *  well-typed arguments and call the Table* functions.
 */

typedef struct sTable *Table;

/* Make a new table mapping "keys" to "values". */
Table MakeTable(void);

/* Enter the mapping "key"->"value" into table "t",
 *    shadowing but not destroying any previous binding for "key". */
void TableEnter(Table t, void *key, void *value);

/* Look up the most recent binding for "key" in table "t" */
void *TableLookup(Table t, void *key);

/* Pop the most recent binding and return its key.
 * This may expose another binding for the same key, if there was one. */
void *TablePop(Table t);


/* Call "show" on every "key"->"value" pair in the table,
 *  including shadowed bindings, in order from the most
 *  recent binding of any key to the oldest binding in the table */
void TableDump(Table t, void (*show)(void *key, void *value));

#endif
