#ifndef tiger_assem_h
#define tiger_assem_h

#include <stdio.h>
#include "util.h"
#include "temp.h"

typedef struct {List/*<TempLabel>*/ labels;} *AS_Targets;
AS_Targets New_AS_Targets(List labels);

typedef struct sASInstr *ASInstr;
struct sASInstr {
	enum {tASOper, tASLabel, tASMove} kind;
	union {
		struct {
			string assem;
			List dst, src;
			AS_Targets jumps;
		} oper;
		struct {string assem; TempLabel label;} label;
		struct {string assem; List dst, src;} move;
	} as;
};

ASInstr AS_Oper(string a, List d, List s, AS_Targets j);
ASInstr AS_Label(string a, TempLabel label);
ASInstr AS_Move(string a, List d, List s);

void AS_print(FILE *out, ASInstr i, TempTable m);
void AS_print_instrs(FILE *out, List insns, TempTable m);

#endif
