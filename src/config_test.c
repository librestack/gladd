#include "config_test.h"
#include "config.h"
#include "minunit.h"
#include <stdio.h>

char *config_test_runner()
{
        printf("Running config tests\n");
        mu_run_test(test_read_config);
        printf("Completed config tests\n");
        return 0;
}

char *test_read_config()
{
        int r;
        
        r = read_config("test.conf");

        mu_assert("reading test.conf", r == 0);

        return 0;
}
