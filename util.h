#ifndef tiger_util_h
#define tiger_util_h

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

typedef char *string;

#define CHECKED_MALLOC(type) (type*)checked_malloc(sizeof(type));
void *checked_malloc(size_t sz);
string String(string);

#endif
