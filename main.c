#include "nodes.h"
#include "interpreter.h"

int main(int argc, char *argv[]) {
    Stmt prog = CompoundStmt(
        // a = 5+3
        AssignStmt("a",
            OpExpr(NumExpr(5), OpPlus, NumExpr(3))
        ),
        // b = print(a, a-1), 10*a;
        // print b;
        CompoundStmt(
            AssignStmt("b",
                SeqExpr(
                    PrintStmt(PairExprList(
                        IdExpr("a"), LastExprList(OpExpr(IdExpr("a"), OpMinus, NumExpr(1))))
                    ),
                    OpExpr(NumExpr(10), OpTimes, IdExpr("a"))
                )
            ),
            PrintStmt(LastExprList(IdExpr("b")))
        )
    );

    (void)Interpret(prog, NULL);
}
