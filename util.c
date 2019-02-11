#include <stdlib.h>
#include <string.h>
#include "util.h"

void *checked_malloc(size_t sz) {
    void *ret = malloc(sz);
    assert(ret);
    return ret;
}

string String(char *s) {
    string p = checked_malloc(strlen(s)+1);
    strcpy(p,s);
    return p;
}
