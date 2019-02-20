#include <stdio.h>
#include <string.h>
#include <stdlib.h> // atoi
#include "util.h"
#include "temp.h"
#include "assem.h"

AS_Targets New_AS_Targets(List labels) {
    AS_Targets p = checked_malloc(sizeof *p);
    p->labels=labels;
    return p;
}

ASInstr AS_Oper(string a, List d, List s, AS_Targets j) {
    ASInstr p = checked_malloc(sizeof *p);
    p->kind = tASOper;
    p->as.oper.assem=a;
    p->as.oper.dst=d;
    p->as.oper.src=s;
    p->as.oper.jumps=j;
    return p;
}

ASInstr AS_Label(string a, TempLabel label) {
    ASInstr p = checked_malloc(sizeof *p);
    p->kind = tASLabel;
    p->as.label.assem=a;
    p->as.label.label=label;
    return p;
}

ASInstr AS_Move(string a, List d, List s) {
    ASInstr p = checked_malloc(sizeof *p);
    p->kind = tASMove;
    p->as.move.assem=a;
    p->as.move.dst=d;
    p->as.move.src=s;
    return p;
}


static Temp nthTemp(List list, int i) {
    assert(list);
    if (i==0) return (Temp)list->data;
    else return nthTemp(list->next,i-1);
}

static TempLabel nthLabel(List list, int i) {
    assert(list);
    if (i==0) return (TempLabel)list->data;
    else return nthLabel(list->next,i-1);
}

static void format(char *result, string assem, List dst,
	List src, AS_Targets jumps, TempTable m) {
	char *p;
	int i = 0; /* offset to result string */
	for (p = assem; p && *p != '\0'; p++)
		if (*p == '`') {
			switch(*(++p)) {
			case 's': {
        int n = atoi(++p);
				string s = TempTableLookup(m, nthTemp(src,n));
        assert(s);
				strcpy(result+i, s);
				i += strlen(s);
        break;
      }
			case 'd': {
        int n = atoi(++p);
				string s = TempTableLookup(m, nthTemp(dst,n));
        assert(s);
				strcpy(result+i, s);
				i += strlen(s);
        break;
      }
			case 'j': {
        assert(jumps);
        int n = atoi(++p);
        string s = LabelString(nthLabel(jumps->labels,n));
        assert(s);
        strcpy(result+i, s);
        i += strlen(s);
        break;
      }
			case '`':
        result[i] = '`'; i++;
				break;
			default: assert(0);
		}
    } else {
        result[i] = *p; i++;
    }
		result[i] = '\0';
}

void AS_print(FILE *out, ASInstr i, TempTable m) {
    char r[200]; /* result */
    switch (i->kind) {
    case tASOper:
        format(r, i->as.oper.assem, i->as.oper.dst, i->as.oper.src, i->as.oper.jumps, m);
        fprintf(out, "%s", r);
        break;
    case tASLabel:
        format(r, i->as.label.assem, NULL, NULL, NULL, m);
        fprintf(out, "%s", r);
        /* i->u.LABEL->label); */
        break;
    case tASMove:
        format(r, i->as.move.assem, i->as.move.dst, i->as.move.src, NULL, m);
        fprintf(out, "%s", r);
        break;
    }
}

void AS_print_instrs(FILE *out, List insns, TempTable m) {
    while (insns) {
        AS_print(out, (ASInstr)insns->data, m);
        insns = insns->next;
    }
}
