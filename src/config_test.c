#include "config_test.h"
#include "config.h"
#include "minunit.h"
#include <stdio.h>

/* test opening config file */
char *test_read_config_000()
{
        mu_assert("reading test.conf", read_config("test.conf") == 0);
        return 0;
}
