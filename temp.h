#ifndef tiger_temp_h
#define tiger_temp_h

#include "util.h"
#include "symbol.h"

struct sTemp {
    int num;
};

typedef struct sTemp *Temp;
Temp NewTemp(void);

typedef Symbol TempLabel;
TempLabel NewLabel(void);
TempLabel NamedLabel(string name);

string LabelString(TempLabel l);

#endif
