#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "config_test.h"
#include "test.h"
 
int tests_run = 0;

static void printline(char *c, int len)
{
        for (; len > 1; len--)
                printf("%s", c);
        printf("\n");
}

static char * all_tests()
{
        printline("*", 80);
        mu_run_test(test_read_config_000);
        mu_run_test(test_read_config_001);
        mu_run_test(test_read_config_002);
        mu_run_test(test_process_config_line_000);
        mu_run_test(test_read_config_003);
        printline("*", 80);
        return 0;
}
 
int main(int argc, char **argv)
{
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
