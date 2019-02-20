#ifndef tiger_codegen_h
#define tiger_codegen_h

#include "util.h"
#include "frame.h"

List/*<AsInsn>*/ Codegen(Frame frame, List stmList/*<IrStmt>*/);

#endif
