#ifndef tiger_util_h
#define tiger_util_h

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

typedef char *string;

struct sList {
    union {
        int i;
        bool b;
        void *data;
    };
    struct sList *next;
};

typedef struct sList *List;

#define CHECKED_MALLOC(type) (type*)checked_malloc(sizeof(type));
void *checked_malloc(size_t sz);
void *checked_realloc(void *p, size_t sz);
string String(string);
string String_format(const char *, ...);
string String_from_int(int n);

List DataList(void *data, List next);
List IntList(int i, List next);
List BoolList(bool b, List next);

List vDataList(int n, ...);
List DataListAppend(List list, void *data);

// append list2 to end of list1
List JoinList(List list1, List list2);
// append new data list element to end of list
List JoinData(List list, void *data);

#endif
