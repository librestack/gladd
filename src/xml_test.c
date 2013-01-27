/*
 * xml_test.c - tests for xml.c
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

#define _GNU_SOURCE
#include "xml_test.h"
#include "xml.h"
#include "config.h"
#include "db.h"
#include "minunit.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *test_xml_doc()
{
        char *xmldoc;

        mu_assert("Beginning test_xml_doc", buildxml(&xmldoc) == 0);
        mu_assert("Verify XML content", 
                strcmp(xmldoc,
                "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n<resources/>\n")
                == 0);
        free(xmldoc);

#ifndef _NPG /* skip the following tests that require postgres */
        db_t *db;
        char *sql;
        char *xmltst = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n<resources><row><id>0</id><name>boris</name></row><row><id>5</id><name>ivan</name></row></resources>\n";

        db = config->dbs->next;
        asprintf(&sql, "SELECT * FROM test;");
        mu_assert("sqltoxml()", sqltoxml(db, sql, NULL, &xmldoc, 0) == 0);
        free(sql);
        fprintf(stderr, "%s\n", xmldoc);
        mu_assert("sqltoxml() - check xml", strcmp(xmldoc, xmltst) == 0);
        free(xmldoc);
#endif /* _NPG */

        return 0;
}
