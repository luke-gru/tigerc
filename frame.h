#ifndef tiger_frame_h
#define tiger_frame_h

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

Temp Frame_fp(void);
IrExpr Frame_Expr(FAccess faccess, IrExpr fp);
IrExpr Frame_ExternalCall(string fnName, List/*<IrExpr>*/ args);
int Frame_Offset(FAccess faccess);

#endif
