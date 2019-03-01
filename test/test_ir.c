#include <stdio.h>
#include <stdlib.h>
#include "test.h"
#include "util.h"
#include "parse.h"
#include "errormsg.h"
#include "semantics.h"
#include "ir_pp.h"

static N_Expr parseFile(const char *fname) {
    return parse((char*)fname);
}

static void assertIREqual(N_Expr prog, const char *expectFile) {
    List fragments = TypeCheck(prog);
    T_ASSERT_EQ(0, EM_errors);
    FILE *f = fopen(expectFile, "r");
    assert(f);
    fclose(f);
    char templateOut[100] = { '\0' };
    const char *template = "/tmp/ir.out.XXXXXX";
    memcpy(templateOut, template, strlen(template));
    int fdOut = mkstemp(templateOut);
    assert(fdOut > 0);
    FILE *fout = fdopen(fdOut, "w");
    PP_Frags(fragments, fout);
    char cmd[4096];
    sprintf(cmd, "diff -u -b -B %s %s", expectFile, templateOut);
    fclose(fout);
    int cmdRes = system(cmd);
    assert(cmdRes >= 0);
    T_ASSERT_EQ(0, cmdRes);
}

static int test_parse_sample(void) {
    N_Expr expr = parseFile("samples/test01.tig");
    T_ASSERT(expr);
    const char *expectFile = "test/ir/test01.out";
    assertIREqual(expr, expectFile);
    return 0;
}

int main(int argc, char *argv[]) {
    INIT_TESTS();
    RUN_TEST(test_parse_sample);
    END_TESTS();
}
