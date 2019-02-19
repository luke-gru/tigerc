#ifndef tiger_canon_h
#define tiger_canon_h

#include "util.h"
#include "temp.h"
#include "ir.h"

/* Let us define canonical trees as having these properties:
 * No SEQ or ESEQ.
 * The parent of each CALL is either EXP(...) or MOVE(TEMP t,...).
 */

struct C_StmtExpr {
    IrStmt s;
    IrExpr e;
};

struct C_Block {
    List/*<IrStmt>*/ stmts;
    TempLabel label; // done label
};

List/*<IrStmt>*/ C_Linearize(IrStmt stmt);
struct C_Block   C_BasicBlocks(List/*<IrStmt>*/ stmts);
// orders cjumps so that false label follows cjump immediately
List/*<IrStmt>*/ C_TraceSchedule(struct C_Block b);


#endif
