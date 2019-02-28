#include "test.h"
#include "ast.h"

static int test_works(void) {
    T_ASSERT(true);
    return 0;
}

int main(int argc, char *argv[]) {
    INIT_TESTS();
    RUN_TEST(test_works);
    END_TESTS();
}
