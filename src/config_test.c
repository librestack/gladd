#include "config_test.h"
#include "config.h"
#include "minunit.h"
#include <stdio.h>


/* process_config_line() must return 1 if line is a comment */
char *test_config_skip_comment()
{
        mu_assert("Ensure comments are skipped by config parser",
                process_config_line("# This line is a comment\n") == 1);
        return 0;
}

/* process_config_line() must return 1 if line is blank */
char *test_config_skip_blank()
{
        mu_assert("Ensure blank lines are skipped by config parser",
                process_config_line(" \t \n") == 1);
        return 0;
}

/* test opening config file */
char *test_config_open_success()
{
        mu_assert("Open test.conf for reading", 
                open_config("test.conf") != NULL);
        return 0;
}

/* ensure failing to open config returns an error */
char *test_config_open_fail()
{
        mu_assert("Ensure failure to open file returns error", 
                read_config("fake.conf") == 1);
        return 0;
}

/* test default value of debug = 0 */
char *test_config_default_debug_value()
{
        mu_assert("Ensure default debug=0", config.debug == 0);
        return 0;
}

/* test default value of port=8080 */
char *test_config_default_port_value()
{
        mu_assert("Ensure default port=8080", config.port == 8080);
        return 0;
}

/* ensure value of debug is set from config */
char *test_config_set_debug_value()
{
        read_config("test.conf");
        mu_assert("Ensure debug is set from config", config.debug == 1);
        return 0;
}

/* ensure value of port is set from config */
char *test_config_set_port_value()
{
        read_config("test.conf");
        mu_assert("Ensure port is set from config", config.port == 3000);
        return 0;
}
