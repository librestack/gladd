/* 
 * Example plugin code
 *
 * tests.c
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
#include "plugin.h"
#include <stdlib.h>
#include <string.h>

char *test_read_input()
{
        FILE *f;

        /* open, read and parse some invalid xml */
        f = fopen("test.invalid.xml", "r");
        mu_assert("read invalid xml", read_input(f) == 0);
        fclose(f);
        mu_assert("parse invalid xml", xml_parse(buf) == XML_STATUS_INVALID);

        /* open, read and parse some valid xml */
        f = fopen("test.xml", "r");
        mu_assert("read valid xml", read_input(f) == 0);
        fclose(f);
        mu_assert("buffer contains request", strncmp(buf,"<request>", 9) == 0);
        mu_assert("parse some valid xml", xml_parse(buf) == XML_STATUS_OK);

        /* fetch value of an element */
        char *value = xml_element("//username");
        mu_assert("perform xpath search for element", value != NULL);
        mu_assert("get value of xml element", strcmp(value, "iamauser") == 0);
        xml_free(value);

        xml_cleanup();

        return 0;
}
