/* 
 * Example plugin code
 *
 * plugin_test.c
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

#include "minunit.h"
#include "tests.h"

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
        mu_run_test(test_read_input);
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
