#ifndef tiger_temp_h
#define tiger_temp_h

#include "util.h"
#include "symbol.h"
#include "table.h"

struct sTemp {
    int num;
};
typedef struct sTemp *Temp;

// map of Temps to their string names (Temp->string)
typedef struct sTable *TempTable;
void TempTableEnter(TempTable m, Temp t, string value);

/* Look up the most recent binding of "sym" in "t", or return NULL
 *    if sym is unbound. */
string TempTableLookup(TempTable m, Temp t);
TempTable NewTempTable(void);
TempTable Temp_NameMap(void);

Temp NewTemp(void);

typedef Symbol TempLabel;
TempLabel NewLabel(void);
TempLabel NamedLabel(string name);

string LabelString(TempLabel l);

#endif
