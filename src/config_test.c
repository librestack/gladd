#include "config_test.h"
#include "config.h"
#include "minunit.h"
#include <stdio.h>


/* test opening config file */
char *test_read_config_000()
{
        mu_assert("Open test.conf for reading", read_config("test.conf") == 0);
        return 0;
}

/* ensure failing to open config returns an error */
char *test_read_config_001()
{
        mu_assert("Ensure failure to open file returns error", 
                read_config("fake.conf") == 1);
        return 0;
}

/* test default value of debug = 0 */
char *test_read_config_002()
{
        mu_assert("Ensure default value of debug=0", config.debug == 0);
        return 0;
}

/* process_config_line() must return 1 if line is a comment */
char *test_process_config_line_000()
{
        mu_assert("Ensure comments are skipped by config parser",
                process_config_line("# This line is a comment\n") == 1);
        return 0;
}

/* test default value of port=8080 */
char *test_read_config_003()
{
        read_config("test.conf");
        mu_assert("Ensure default port=8080", config.port == 8080);
        return 0;
}

/* test */
char *test_read_config_004()
{
        read_config("test.conf");
        mu_assert("Ensure port is set from config", config.port == 3000);
        return 0;
}
