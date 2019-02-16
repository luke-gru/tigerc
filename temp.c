#include <stdio.h>
#include "util.h"
#include "temp.h"

static int _temps = 0;
static int _labels = 0;

Temp NewTemp(void) {
    Temp p = CHECKED_MALLOC(struct sTemp);
    p->num = _temps++;
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
