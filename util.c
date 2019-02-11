#include <stdlib.h>
#include "util.h"

void *checked_malloc(size_t sz) {
    void *ret = malloc(sz);
    assert(ret);
    return ret;
}
