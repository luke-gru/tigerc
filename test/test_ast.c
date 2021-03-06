#include <stdio.h>
#include <stdlib.h>
#include "test.h"
#include "parse.h"
#include "ast.h"
#include "print_ast.h"

static N_Expr parseFile(const char *fname) {
    return parse((char*)fname);
}

static void assertASTEqual(N_Expr prog, const char *expectFile) {
    FILE *f = fopen(expectFile, "r");
    assert(f);
    fclose(f);
    char templateOut[100] = { '\0' };
    const char *template = "/tmp/ast.out.XXXXXX";
    memcpy(templateOut, template, strlen(template));
    int fdOut = mkstemp(templateOut);
    assert(fdOut > 0);
    FILE *fout = fdopen(fdOut, "w");
    pr_exp(fout, prog, -1);
    char cmd[4096];
    sprintf(cmd, "diff -u -b %s %s", expectFile, templateOut);
    fclose(fout);
    int cmdRes = system(cmd);
    assert(cmdRes >= 0);
    T_ASSERT_EQ(0, cmdRes);
}

static int test_parse_sample(void) {
    N_Expr expr = parseFile("samples/test01.tig");
    T_ASSERT(expr);
    const char *expectFile = "test/ast/test01.out";
    assertASTEqual(expr, expectFile);
    return 0;
}

int main(int argc, char *argv[]) {
    INIT_TESTS();
    RUN_TEST(test_parse_sample);
    END_TESTS();
}
