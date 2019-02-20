#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "util.h"

void *checked_malloc(size_t sz) {
    void *ret = malloc(sz);
    assert(ret);
    return ret;
}

void *checked_realloc(void *p, size_t size) {
    void *ptr = realloc(p, size);
    assert(ptr);
    return ptr;
}

string String(char *s) {
    string p = checked_malloc(strlen(s)+1);
    strcpy(p,s);
    return p;
}

#define BUFSIZE 1024
string String_from_int(int n) {
    // wastes memory, but that's okay
    char *str = checked_malloc(sizeof(*str) * (BUFSIZE + 1));
    snprintf(str, BUFSIZE + 1, "%d", n);
    return str;
}

string String_format(const char *s, ...) {
    char buffer[BUFSIZE], *result = NULL;
    const char *p = s; /* cursor pointer */
    const char *str = NULL; /* pointer to variable argument strings */
    int len = 0; /* size of result */
    int i = 0; /* size of buffer */
    int n = 0; /* length of variable argument strings */
    bool isDigit = false; /* needed so we can free memory allocated to string */
    va_list ap;
    va_start(ap, s);
    for (; *p; p++) {
        if (*p == '%') {
            switch (*++p) {
                case 's':
                    str = va_arg(ap, const char *);
                    break;
                case 'd':
                    str = String_from_int(va_arg(ap, int));
                    isDigit = true;
                    break;
                default:
                    assert(0); /* Invalid format specifier */
            }
            n = strlen(str);
        } else {
            if (i < BUFSIZE - 1) {
                buffer[i++] = *p; continue;
            } else {
                str = p;
                n = 1;
            }
        }
        if (i + n > BUFSIZE) {
            result = checked_realloc(result, sizeof(*result) * (len + i + 1));
            if (len > 0) strncat(result, buffer, i);
            else strncpy(result, buffer, i);
            len += i;
            i = 0;
        }
        strncpy(buffer + i, str, n);
        i += n;
        if (isDigit) { free((void *)str); str = NULL; isDigit = false; }
    }
    if (i > 0) {
        result = checked_realloc(result, sizeof(*result) * (len + i + 1));
        if (len > 0) strncat(result, buffer, i);
        else strncpy(result, buffer, i);
        /* can forget about i and len here, since we are exiting */
    }
    va_end(ap);
    return result;
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
