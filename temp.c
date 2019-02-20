#include <stdio.h>
#include "util.h"
#include "temp.h"

// start with 100 so that we can replace them easier later in the outputted
// asm with register names, which are usually 3 letters.
static int _temps = 100;
static int _labels = 0;

Temp NewTemp(void) {
    Temp p = CHECKED_MALLOC(struct sTemp);
    p->num = _temps++;
    {
        char r[16];
        sprintf(r, "%d", p->num);
        TempTableEnter(Temp_NameMap(), p, String(r));
    }
    return p;
}

TempLabel NewLabel(void) {
    char buf[16];
    snprintf(buf, sizeof(buf), ".L%d", _labels++);
    return NamedLabel(String(buf));
}

TempLabel NamedLabel(string name) {
    return GetSym(name);
}

string LabelString(TempLabel l) {
    return SymName(l);
}

void TempTableEnter(TempTable m, Temp t, string value) {
    TableEnter(m, t, value);
}

string TempTableLookup(TempTable m, Temp t) {
  return TableLookup(m, t);
}

TempTable NewTempTable(void) {
    return MakeTable();
}

TempTable Temp_NameMap(void) {
    static TempTable nameMap = NULL;
    if (nameMap == NULL) {
        nameMap = NewTempTable();
    }
    return nameMap;
}
