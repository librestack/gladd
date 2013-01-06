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
        printf("Running tests\n");
        printline("*", 80);
        mu_run_test(test_config_skip_comment);
        mu_run_test(test_config_skip_blank);
        mu_run_test(test_config_invalid_line);
        mu_run_test(test_config_default_debug_value);
        mu_run_test(test_config_default_port_value);
        mu_run_test(test_config_open_fail);
        mu_run_test(test_config_open_success);
        mu_run_test(test_config_set_debug_value);
        mu_run_test(test_config_set_port_value);
        mu_run_test(test_config_read_url_static);
        mu_run_test(test_config_read_url_static_next);
        printline("*", 80);
        return 0;
}
 
int main(int argc, char **argv)
{
        char *result = all_tests();
        if (result != 0) {
                printline("*", 80);
                printf("FIXME: %s\n", result);
                printline("*", 80);
        }
        else {
                printf("ALL TESTS PASSED\n");
        }
        printf("Tests run: %d\n", tests_run);
 
        return result != 0;
}
