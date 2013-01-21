/* 
 * db_test.c - unit tests for config.c
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

#include "db_test.h"
#include "minunit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *test_dbs()
{
        db_t *db;
        db = config->dbs;

#ifndef _NPG /* skip postgres tests */
        mu_assert("Ensure db_connect() fails when db doesn't exist",
                db_connect(db) == 1);
        mu_assert("db_disconnect()", db_disconnect(db) == 0);
#endif /* _NPG */

        mu_assert("Get first test database from config", db = db->next);
        while (db != NULL) {
                mu_run_test(test_db, db);
                db = db->next;
        }

        return 0;
}

char *test_db(db_t *db)
{
        row_t *r;
        int rowc;

#ifdef _NPG /* skip postgres tests */
        if (strcmp(db->type, "pg") == 0) {
                mu_assert("*** Skipping postgresql tests ***", 0 == 0);
                return 0;
        }
#endif /* _NPG */
#ifdef _NMY /* skip mysql tests */
        if (strcmp(db->type, "my") == 0) {
                mu_assert("*** Skipping mysql tests ***", 0 == 0);
                return 0;
        }
#endif /* _NMY */
#ifdef _NLDAP
        if (strcmp(db->type, "ldap") == 0) {
                mu_assert("*** Skipping ldap tests ***", 0 == 0);
                return 0;
        }
#endif /* _NLDAP */

        mu_assert("db_connect()", db_connect(db) == 0);
        mu_assert("Test database connection", db->conn != NULL);

        if (strcmp(db->type, "ldap") != 0) {

        mu_assert("db_exec_sql() invalid sql returns failure",
                db_exec_sql(db, "invalidsql") != 0);
        mu_assert("db_exec_sql() BEGIN",
                db_exec_sql(db, "BEGIN") == 0);
        mu_assert("db_exec_sql() DROP TABLE",
                db_exec_sql(db, "DROP TABLE IF EXISTS test") == 0);
        mu_assert("db_exec_sql() CREATE TABLE",
                db_exec_sql(db,
                "CREATE TABLE IF NOT EXISTS test(id integer, name char(32))")
                == 0);
        mu_assert("db_exec_sql() INSERT",
                db_exec_sql(db,
                "INSERT INTO test(id, name) VALUES (0, 'boris')") == 0);
        mu_assert("db_exec_sql() INSERT",
                db_exec_sql(db,
                "INSERT INTO test(id, name) VALUES (5, 'ivan')") == 0);
        mu_assert("db_exec_sql() COMMIT",
                db_exec_sql(db, "COMMIT") == 0);

        mu_assert("db_fetch_all() SELECT",
                db_fetch_all(db, "SELECT * FROM test", &r, &rowc) == 0);

        field_t *f;

        mu_assert("Check rows are populated", r != NULL);
        mu_assert("Got results", rowc != 0);
        mu_assert("Get 1st row", f = r->fields);
        mu_assert("Check 1st field name", strcmp(f->fname, "id") == 0);
        mu_assert("Check 1st field value", strcmp(f->fvalue, "0") == 0);
        mu_assert("Get next field", f = f->next);
        mu_assert("Check 2nd field name", strcmp(f->fname, "name") == 0);
        mu_assert("Check 2nd field value", strncmp(f->fvalue, "boris",5) == 0);
        mu_assert("Ensure last field->next == NULL", f->next == NULL);
        mu_assert("Get 2nd row", f = r->next->fields);
        mu_assert("Check 1st field name", strcmp(f->fname, "id") == 0);
        mu_assert("Check 1st field value", strcmp(f->fvalue, "5") == 0);
        mu_assert("Get next field", f = f->next);
        mu_assert("Check 2nd field name", strcmp(f->fname, "name") == 0);
        mu_assert("Check 2nd field value", strncmp(f->fvalue, "ivan", 4) == 0);
        mu_assert("Ensure last field->next == NULL", f->next == NULL);

        }
        mu_assert("db_disconnect()", db_disconnect(db) == 0);

        liberate_rows(r);

        return 0;
}
