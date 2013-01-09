/*
 * test.c
 *
 * this file is part of GLADD
 *
 * Copyright (c) 2012, 2013 Brett Sheffield <brett@gladserv.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see the file COPYING in the distribution).
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "args_test.h"
#include "auth_test.h"
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
        mu_run_test(test_config_defaults);
        mu_run_test(test_config_open_fail);
        mu_run_test(test_config_open_success);
        mu_run_test(test_config_set_debug_value);
        mu_run_test(test_config_set_port_value);
        mu_run_test(test_config_read_url_static);
        mu_run_test(test_config_read_url_static_next);
        mu_run_test(test_args_invalid);
        mu_run_test(test_auth_default);
        mu_run_test(test_auth_deny);
        mu_run_test(test_auth_allow);
        mu_run_test(test_config_add_acl_invalid);
        mu_run_test(test_config_acl_allow_all);
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
