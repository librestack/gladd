/* 
 * db.h - code to handle database connections etc.
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

#ifndef __GLADD_DB_H__
#define __GLADD_DB_H__ 1

#include "config.h"

typedef struct field_t {
        char *fname;
        char *fvalue;
        struct field_t *next;
} field_t;

typedef struct row_t {
        struct field_t *fields;
        struct row_t *next;
} row_t;

int db_connect(db_t *db);
int db_connect_ldap(db_t *db);
int db_connect_my(db_t *db);
int db_connect_pg(db_t *db);
int db_create(db_t *db);
int db_create_pg(db_t *db);
int db_disconnect(db_t *db);
int db_disconnect_my(db_t *db);
int db_disconnect_pg(db_t *db);
int db_exec_sql(db_t *db, char *sql);
int db_exec_sql_my(db_t *db, char *sql);
int db_exec_sql_pg(db_t *db, char *sql);
int db_fetch_all(db_t *db, char *sql, row_t **rows, int *rowc);
int db_fetch_all_my(db_t *db, char *sql, row_t **rows, int *rowc);
int db_fetch_all_pg(db_t *db, char *sql, row_t **rows, int *rowc);
void free_fields(field_t *f);
void liberate_rows(row_t *r);

#endif /* __GLADD_DB_H__ */
