#ifndef tiger_frame_h
#define tiger_frame_h

#include "util.h"
#include "temp.h"

struct sFrame;  // machine dependent
struct sFAccess; // machine dependent, either reg or stack

typedef struct sFrame *Frame;
typedef struct sFAccess *FAccess;

Frame NewFrame(TempLabel name, List formalEscapes);
TempLabel FrameName(Frame fr);
List FrameFormals(Frame fr);
FAccess FrameAllocLocal(Frame fr, bool escape);

#endif
