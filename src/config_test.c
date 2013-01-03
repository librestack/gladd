#include "config_test.h"
#include "config.h"
#include "minunit.h"
#include <stdio.h>

/* test opening config file */
char *test_read_config_000(char *configfile)
{
        mu_assert("Open test.conf for reading", read_config("test.conf") == 0);
        return 0;
}

/* ensure failing to open config returns an error */
char *test_read_config_001(char *configfile)
{
        mu_assert("Ensure failure to open file returns error", 
                read_config("fake.conf") == 1);
        return 0;
}

