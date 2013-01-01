#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "config_test.h"
#include "test.h"
 
int tests_run = 0;
 
static char * all_tests() {
        config_test_runner();
        return 0;
}
 
int main(int argc, char **argv) {
        char *result = all_tests();
        if (result != 0) {
                printf("%s\n", result);
        }
        else {
                printf("ALL TESTS PASSED\n");
        }
        printf("Tests run: %d\n", tests_run);
 
        return result != 0;
}
