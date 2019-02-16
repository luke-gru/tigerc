/*
 * errormsg.c - functions used in all phases of the compiler to give
 *              error messages about the Tiger program.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "util.h"
#include "errormsg.h"

bool anyErrors = false;
static string fileName = (string)"";
static int lineNum = 1;

int EM_tokPos = 0;
int EM_errors = 0;

extern FILE *yyin;

static List linePos = NULL;

void EM_newline(void) {
    lineNum++;
    linePos = IntList(EM_tokPos, linePos);
}

void EM_error(int pos, char *message, ...) {
    va_list ap;
    List lines = linePos;
    int num = lineNum;

    anyErrors = true;
    while (lines && lines->i >= pos) {
        lines = lines->next;
        num--;
    }

    if (fileName) fprintf(stderr, "%s:", fileName);
    if (lines) fprintf(stderr, "%d.%d: ", num, pos - lines->i);
    va_start(ap,message);
    vfprintf(stderr, message, ap);
    va_end(ap);
    fprintf(stderr,"\n");
    EM_errors++;
}

void EM_reset(string fname) {
    anyErrors = false;
    EM_errors = 0;
    fileName = fname;
    lineNum = 1;
    linePos = IntList(0, NULL);
    yyin = fopen(fname,"r");
    if (!yyin) {
        EM_error(0, "cannot open");
        exit(1);
    }
}

void EM_fset(FILE *f, string fname) {
    anyErrors = false;
    EM_errors = 0;
    fileName = fname;
    lineNum = 1;
    linePos = IntList(0, NULL);
    yyin = f;
}
