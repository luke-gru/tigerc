#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
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

List DataList(void *data, List next) {
    List p = CHECKED_MALLOC(struct sList);
    p->data = data;
    p->next = next;
    return p;
}

List IntList(int i, List next) {
    List p = CHECKED_MALLOC(struct sList);
    p->i = i;
    p->next = next;
    return p;
}

List BoolList(bool b, List next) {
    List p = CHECKED_MALLOC(struct sList);
    p->b = b;
    p->next = next;
    return p;
}

List vDataList(int n, ...) {
    List result = NULL, next = NULL;
    va_list ap;

    va_start(ap, n);
    for (; n > 0; n--) {
        void *data = va_arg(ap, void *);
        List p = DataList(data, NULL);
        if (result) {
            next = next->next = p;
        } else {
            result = next = p;
        }
    }
    return result;
}

// join list2 to end of list1
List JoinList(List list1, List list2) {
    List p = list1;
    if (!p) {
        return list2;
    }
    while (p->next) {
        p = p->next;
    }
    p->next = list2;
    return list1;
}

List DataListAppend(List list, void *data) {
    if (!list) {
        return DataList(data, NULL);
    } else {
        while (list->next) {
            list = list->next;
        }
        list->next = DataList(data, NULL);
        return list;
    }
}

List JoinData(List list, void *data) {
    return JoinList(list, DataList(data, NULL));
}
