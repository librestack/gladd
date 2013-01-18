/* 
 * db.c - code to handle database connections etc.
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
#include <libpq-fe.h>
#include <mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include "db.h"

/* connect to specified database 
 * pointer to the connection is stored in db_t struct 
 * a wrapper for the database-specific functions
 */
int db_connect(db_t *db)
{
        if (db == NULL) {
                fprintf(stderr, "No database info supplied to db_connect()\n");
                return -1;
        }
        if (strcmp(db->type, "pg") == 0) {
                return db_connect_pg(db);
        }
        else if (strcmp(db->type, "my") == 0) {
                return db_connect_my(db);
        }
        else {
                fprintf(stderr, 
                        "Invalid database type '%s' passed to db_connect()\n", 
                        db->type);
                return -1;
        }
        return 0;
}

/* connect to mysql database */
int db_connect_my(db_t *db)
{
        MYSQL *conn;

        conn = mysql_init(NULL);
        if (conn == NULL) {
                fprintf(stderr, "%u: %s\n", mysql_errno(conn),
                                            mysql_error(conn));
                return -1;
        }

        if (mysql_real_connect(conn, db->host, db->user, db->pass, db->db,
                                                        0, NULL, 0) == NULL)
        {
                fprintf(stderr, "%u: %s\n", mysql_errno(conn),
                                            mysql_error(conn));
                return -1;
        }

        db->conn = conn;

        return 0;
}

/* connect to a postgresql database */
/* FIXME: memory leak in here, despite PQfinish() */
int db_connect_pg(db_t *db)
{
        char *conninfo;
        PGconn *conn;
        int retval = 0;

        if (asprintf(&conninfo,
                "host = %s dbname = %s", db->host, db->db) == -1)
        {
                fprintf(stderr, 
                        "asprintf failed for conninfo in db_connect_pg()\n");
                return -2;
        }
        conn = PQconnectdb(conninfo);
        free(conninfo);
        if (PQstatus(conn) != CONNECTION_OK)
        {
                syslog(LOG_ERR, "connection to database failed.\n");
                syslog(LOG_DEBUG, "%s", PQerrorMessage(conn));
                fprintf(stderr, "%s", PQerrorMessage(conn));
                retval = PQstatus(conn);
                PQfinish(conn);
                return retval;
        }
        db->conn = conn;
        return 0;
}

/* wrapper for the database-specific db creation functions */
int db_create(db_t *db)
{
        if (db == NULL) {
                fprintf(stderr, 
                        "No database info supplied to db_create()\n");
                return -1;
        }
        if (strcmp(db->type, "pg") == 0) {
                return db_create_pg(db);
        }
        else {
                fprintf(stderr, 
                    "Invalid database type '%s' passed to db_create()\n",
                    db->type);
        }
        return 0;
}

/* TODO: db_create_pg() */
int db_create_pg(db_t *db)
{
        return 0;
}

/* wrapper for the database-specific disconnect functions */
int db_disconnect(db_t *db)
{
        if (db == NULL) {
                fprintf(stderr, 
                        "No database info supplied to db_disconnect()\n");
                return -1;
        }
        if (strcmp(db->type, "pg") == 0) {
                return db_disconnect_pg(db);
        }
        else if (strcmp(db->type, "my") == 0) {
                return db_disconnect_my(db);
        }
        else {
                fprintf(stderr, 
                    "Invalid database type '%s' passed to db_disconnect()\n",
                    db->type);
        }
        return 0;
}

/* disconnect from a mysql db */
int db_disconnect_my(db_t *db)
{
        mysql_close(db->conn);

        return 0;
}

/* disconnect from a postgresql db */
int db_disconnect_pg(db_t *db)
{
        PQfinish(db->conn);

        return 0;
}

/* execute some sql on a database
 * wrapper for db-specific functions */
int db_exec_sql(db_t *db, char *sql)
{
        if (db == NULL) {
                fprintf(stderr, 
                        "No database info supplied to db_exec_sql()\n");
                return -1;
        }
        if (strcmp(db->type, "pg") == 0) {
                return db_exec_sql_pg(db, sql);
        }
        else if (strcmp(db->type, "my") == 0) {
                return db_exec_sql_my(db, sql);
        }
        else {
                fprintf(stderr, 
                    "Invalid database type '%s' passed to db_exec_sql()\n",
                    db->type);
        }
        return 0;
}

/* execute sql against a mysql db */
int db_exec_sql_my(db_t *db, char *sql)
{
        if (mysql_query(db->conn, sql) != 0) {
                fprintf(stderr, "%u: %s\n", mysql_errno(db->conn), 
                                            mysql_error(db->conn));
                return -1;
        }
        return 0;
}

/* execute sql against a postgresql db */
int db_exec_sql_pg(db_t *db, char *sql)
{
        PGresult *res;

        res = PQexec(db->conn, sql);
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                fprintf(stderr,
                        "SQL exec failed: %s", PQerrorMessage(db->conn));
                PQclear(res);
                return -1;
        }
        PQclear(res);

        return 0;
}

/* return all results from a cursor
 * wrapper for db-specific functions */
int db_fetch_all(db_t *db, char *sql, row_t **rows, int *rowc)
{
        if (db == NULL) {
                fprintf(stderr, 
                        "No database info supplied to db_fetch_all()\n");
                return -1;
        }
        if (strcmp(db->type, "pg") == 0) {
                return db_fetch_all_pg(db, sql, rows, rowc);
        }
        else if (strcmp(db->type, "my") == 0) {
                return db_fetch_all_my(db, sql, rows, rowc);
        }
        else {
                fprintf(stderr, 
                    "Invalid database type '%s' passed to db_fetch_all()\n",
                    db->type);
        }
        return 0;
}

/* return all results from a cursor - postgres */
int db_fetch_all_my(db_t *db, char *sql, row_t **rows, int *rowc)
{
        mysql_close(db->conn);
        return db_exec_sql_my(db, sql);
        /*
        if (mysql_store_result(db->conn) != 0) {
                fprintf(stderr, "%u: %s\n", mysql_errno(db->conn), 
                                            mysql_error(db->conn));
                return -1;
        }
        return 0;
        */
}

/* return all results from a cursor - postgres */
int db_fetch_all_pg(db_t *db, char *sql, row_t **rows, int *rowc)
{
        PGresult *res;
        int i, j;
        int nFields;
        field_t *f;
        field_t *ftmp = NULL;
        row_t *r;
        row_t *rtmp = NULL;

        res = PQexec(db->conn, sql);
        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
                fprintf(stderr, "query failed: %s", 
                        PQerrorMessage(db->conn));
                return -1;
        }

        /* populate rows and fields */
        nFields = PQnfields(res);
        for (j = 0; j < PQntuples(res); j++) {
                r = malloc(sizeof(row_t));
                r->fields = NULL;
                r->next = NULL;
                for (i = 0; i < nFields; i++) {
                        f = malloc(sizeof(field_t));
                        f->fname = strdup(PQfname(res, i));
                        f->fvalue = strdup(PQgetvalue(res, j, i));
                        f->next = NULL;
                        if (r->fields == NULL) {
                                r->fields = f;
                        }
                        if (ftmp != NULL) {
                                ftmp->next = f;
                        }
                        ftmp = f;
                }
                if (rtmp == NULL) {
                        /* as this is our first row, update the ptr */
                        *rows = r;
                }
                else {
                        rtmp->next = r;
                }
                ftmp = NULL;
                rtmp = r;
        }
        *rowc = PQntuples(res);

        PQclear(res);

        return 0;
}

/* free field_t struct */
void free_fields(field_t *f)
{
        field_t *next_f;
        while (f != NULL) {
                free(f->fname);
                free(f->fvalue);
                next_f = f->next;
                free(f);
                f = next_f;
        }
}

/* free row_t struct */
void free_rows(row_t *r)
{
        row_t *next_r;
        while (r != NULL) {
                free_fields(r->fields);
                next_r = r->next;
                free(r);
                r = next_r;
        }
}
