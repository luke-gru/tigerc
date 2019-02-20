#ifndef tiger_frame_h
#define tiger_frame_h

#include <stdio.h>
#include "util.h"
#include "temp.h"
#include "ir.h"

extern const int FRAME_WORD_SIZE;

struct sFrame;  // machine dependent
struct sFAccess; // machine dependent, either reg or stack

typedef struct sFrame *Frame;
typedef struct sFAccess *FAccess;

Frame NewFrame(TempLabel name, List formalEscapes);
TempLabel FrameName(Frame fr);
List FrameFormals(Frame fr);
FAccess FrameAllocLocal(Frame fr, bool escape);

List Frame_caller_saves(void);
bool Frame_doesEscape(FAccess faccess);

Temp Frame_fp(void);
Temp Frame_rv(void);
IrExpr Frame_Expr(FAccess faccess, IrExpr fp);
IrExpr Frame_ExternalCall(string fnName, List/*<IrExpr>*/ args);
int Frame_Offset(FAccess faccess);

IrStmt Frame_Proc_Entry_Exit_1(Frame fr, IrStmt stmt);

// needs access to frame internals, so needs to go here
void PP_Frags(List/*<Frag>*/ frags, FILE *out);


#endif
