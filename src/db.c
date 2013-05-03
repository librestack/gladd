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
#define LDAP_DEPRECATED 1
#include "db.h"
#include "http.h"
#include <ldap.h>
#include <libpq-fe.h>
#include <mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

/* connect to specified database 
 * pointer to the connection is stored in db_t struct 
 * a wrapper for the database-specific functions
 */
int db_connect(db_t *db)
{
        if (db == NULL) {
                syslog(LOG_ERR, "No database info supplied to db_connect()\n");
                return -1;
        }
        if (strcmp(db->type, "pg") == 0) {
                return db_connect_pg(db);
        }
        else if (strcmp(db->type, "my") == 0) {
                return db_connect_my(db);
        }
        else if (strcmp(db->type, "ldap") == 0) {
                return db_connect_ldap(db);
        }
        else {
                syslog(LOG_ERR, 
                        "Invalid database type '%s' passed to db_connect()\n", 
                        db->type);
                return -1;
        }
        return 0;
}

/* connect to ldap */
int db_connect_ldap(db_t *db)
{
        LDAP *l;
        int rc;
        int protocol = LDAP_VERSION3;

        rc = ldap_initialize(&l, db->host);
        if (rc != LDAP_SUCCESS) {
                syslog(LOG_DEBUG,
                  "Could not create LDAP session handle for URI=%s (%d): %s\n",
                  db->host, rc, ldap_err2string(rc));
                return -1;
        }
        syslog(LOG_DEBUG, "ldap_initialise() successful");

        /* Ensure we use LDAP v3 */
        rc = ldap_set_option(l, LDAP_OPT_PROTOCOL_VERSION, &protocol);

        db->conn = l;

        return 0;
}

/* connect to mysql database */
int db_connect_my(db_t *db)
{
        MYSQL *conn;

        conn = mysql_init(NULL);
        if (conn == NULL) {
                syslog(LOG_ERR, "%u: %s\n", mysql_errno(conn),
                                            mysql_error(conn));
                return -1;
        }

        if (mysql_real_connect(conn, db->host, db->user, db->pass, db->db,
                                                        0, NULL, 0) == NULL)
        {
                syslog(LOG_ERR, "%u: %s\n", mysql_errno(conn),
                                            mysql_error(conn));
                return -1;
        }
        db->conn = conn;

        return 0;
}

/* connect to a postgresql database */
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
        else if (strcmp(db->type, "ldap") == 0) {
                return db_disconnect_ldap(db);
        }
        else {
                fprintf(stderr, 
                    "Invalid database type '%s' passed to db_disconnect()\n",
                    db->type);
                return -1;
        }
        return 0;
}

/* disconnect from a mysql db */
int db_disconnect_ldap(db_t *db)
{
        int rc;
        rc = ldap_unbind(db->conn);
        return 0;
}

/* disconnect from a mysql db */
int db_disconnect_my(db_t *db)
{
        mysql_close(db->conn);
        mysql_library_end();

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
        int isconn = 0;

        if (db == NULL) {
                fprintf(stderr, 
                        "No database info supplied to db_exec_sql()\n");
                return -1;
        }

        /* connect if we aren't already */
        if (db->conn == NULL) {
                if (db_connect(db) != 0) {
                        syslog(LOG_ERR, "Failed to connect to db on %s",
                                db->host);
                        return -1;
                }
                isconn = 1;
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

        /* leave the connection how we found it */
        if (isconn == 1)
                db_disconnect(db);

        return 0;
}

/* execute sql against a mysql db */
int db_exec_sql_my(db_t *db, char *sql)
{
        if (mysql_query(db->conn, sql) != 0) {
                syslog(LOG_DEBUG, "%u: %s\n", mysql_errno(db->conn), 
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
                syslog(LOG_DEBUG,
                       "SQL exec failed: %s", PQerrorMessage(db->conn));
                PQclear(res);
                return -1;
        }
        PQclear(res);

        return 0;
}

/* return all results from a SELECT
 * wrapper for db-specific functions */
int db_fetch_all(db_t *db, char *sql, field_t *filter, row_t **rows, int *rowc)
{
        if (db == NULL) {
                syslog(LOG_ERR, 
                        "No database info supplied to db_fetch_all()\n");
                return -1;
        }
        if (strcmp(db->type, "pg") == 0) {
                return db_fetch_all_pg(db, sql, filter, rows, rowc);
        }
        else if (strcmp(db->type, "my") == 0) {
                return db_fetch_all_my(db, sql, filter, rows, rowc);
        }
        else if (strcmp(db->type, "ldap") == 0) {
                return db_fetch_all_ldap(db, sql, filter, rows, rowc);
        }
        else {
                syslog(LOG_ERR, 
                    "Invalid database type '%s' passed to db_fetch_all()\n",
                    db->type);
                return -1;
        }
        return 0;
}

int db_fetch_all_ldap(db_t *db, char *query, field_t *filter, row_t **rows,
        int *rowc)
{
        BerElement *ber;
        LDAPMessage *msg;
        LDAPMessage *res = NULL;
        char *a;
        char *lfilter = NULL;
        char *search;
        char **vals;
        int i;
        int rc;
        field_t *f;
        field_t *ftmp = NULL;
        row_t *r;
        row_t *rtmp = NULL;

        asprintf(&search, "%s,%s", query, db->db);
        rc = ldap_search_ext_s(db->conn, search, LDAP_SCOPE_SUBTREE,
                lfilter, NULL, 0, NULL, NULL, LDAP_NO_LIMIT,
                LDAP_NO_LIMIT, &res);
        free(search);

        if (rc != LDAP_SUCCESS) {
                syslog(LOG_DEBUG, "search error: %s (%d)",
                                ldap_err2string(rc), rc);
                return -1;
        }
        syslog(LOG_DEBUG, "ldap_search_ext_s successful");

        *rowc = ldap_count_messages(db->conn, res);
        syslog(LOG_DEBUG, "Messages: %i", *rowc);

        /* populate rows and fields */
        for (msg = ldap_first_entry(db->conn, res); msg != NULL;
        msg = ldap_next_entry(db->conn, msg))
        {
                r = malloc(sizeof(row_t));
                r->fields = NULL;
                r->next = NULL;
                for (a = ldap_first_attribute(db->conn, msg, &ber); a != NULL;
                a = ldap_next_attribute(db->conn, msg, ber))
                {
                        /* attributes may have more than one value - here
                           we list them all as separate fields */
                        if ((vals = ldap_get_values(db->conn, msg, a)) != NULL)
                        {
                                for (i = 0; vals[i] != NULL; i++) {
                                        f = malloc(sizeof(field_t));
                                        f->fname = strdup(a);
                                        f->fvalue = strdup(vals[i]);
                                        f->next = NULL;
                                        if (r->fields == NULL) {
                                                r->fields = f;
                                        }
                                        if (ftmp != NULL) {
                                                ftmp->next = f;
                                        }
                                        ftmp = f;
                                }
                                ldap_value_free(vals);
                        }
                        ldap_memfree(a);
                }
                ldap_memfree(ber);
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
        ldap_msgfree(res);
        return 0;
}

/* return all results from a SELECT - postgres */
int db_fetch_all_my(db_t *db, char *sql, field_t *filter, row_t **rows,
        int *rowc)
{
        MYSQL_RES *res;
        MYSQL_ROW row;
        MYSQL_FIELD *fields;
        int i;
        int nFields;
        field_t *f;
        field_t *ftmp = NULL;
        row_t *r;
        row_t *rtmp = NULL;
        char *sqltmp;
        char *join;

        *rowc = 0;

        if (filter != NULL) {
                join = (strcasestr(sql, "WHERE") == NULL) ? "WHERE" : "AND";
                sqltmp = strdup(sql);
                asprintf(&sql, "%s %s %s=\"%s\"", sqltmp, join, filter->fname,
                        filter->fvalue);
                free(sqltmp);
        }

        if (mysql_query(db->conn, sql) != 0) {
                syslog(LOG_ERR, "%u: %s\n", mysql_errno(db->conn), 
                                            mysql_error(db->conn));
                return -1;
        }

        res = mysql_store_result(db->conn);
        *rowc = mysql_num_rows(res);

        /* populate rows and fields */
        fields = mysql_fetch_fields(res);
        nFields = mysql_num_fields(res);
        while ((row = mysql_fetch_row(res))) {
                r = malloc(sizeof(row_t));
                r->fields = NULL;
                r->next = NULL;
                for (i = 0; i < nFields; i++) {
                        f = malloc(sizeof(field_t));
                        f->fname = strdup(fields[i].name);
                        asprintf(&f->fvalue, "%s", row[i] ? row[i] : "NULL");
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
        mysql_free_result(res);

        return 0;
}

/* return all results from a SELECT - postgres */
int db_fetch_all_pg(db_t *db, char *sql, field_t *filter, row_t **rows,
        int *rowc)
{
        PGresult *res;
        int i, j;
        int nFields;
        field_t *f;
        field_t *ftmp = NULL;
        row_t *r;
        row_t *rtmp = NULL;
        char *sqltmp;

        if (filter != NULL) {
                sqltmp = strdup(sql);
                asprintf(&sql, "%s WHERE %s='%s'", sqltmp, filter->fname,
                        filter->fvalue);
                free(sqltmp);
        }

        res = PQexec(db->conn, sql);
        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
                syslog(LOG_ERR, "query failed: %s", 
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

/* database agnostic resource insertion */
int db_insert(db_t *db, char *resource, keyval_t *data)
{
        if (db == NULL) {
                syslog(LOG_ERR, 
                        "No database info supplied to db_insert()\n");
                return -1;
        }
        /* TODO: field validation */
        if ((strcmp(db->type, "pg") == 0) || (strcmp(db->type, "my") == 0)) {
                return db_insert_sql(db, resource, data);
        }
        else if (strcmp(db->type, "ldap") == 0) {
                return db_insert_ldap(db, resource, data);
        }
        else {
                syslog(LOG_ERR, 
                    "Invalid database type '%s' passed to db_insert()\n",
                    db->type);
                return -1;
        }
        return 0;
}

/* ldap add */
int db_insert_ldap(db_t *db, char *resource, keyval_t *data)
{
        /* TODO */
        return 0;
}

/* INSERT into sql database */
int db_insert_sql(db_t *db, char *resource, keyval_t *data)
{
        char *flds = NULL;
        char *sql;
        char *vals = NULL;
        char *tmpflds = NULL;
        char *tmpvals = NULL;
        char quot = '\'';
        int rval;
        int isconn = 0;
      
        /* use backticks to quote mysql */
        if (strcmp(db->type, "my") == 0)
                quot = '"';
                
        /* build INSERT sql from supplied data */
        while (data != NULL) {
                fprintf(stderr, "%s = %s\n", data->key, data->value);
                if (flds == NULL) {
                        asprintf(&flds, "%s", data->key);
                        asprintf(&vals, "%1$c%2$s%1$c", quot, data->value);
                }
                else {
                        tmpflds = strdup(flds);
                        tmpvals = strdup(vals);
                        free(flds); free(vals);
                        asprintf(&flds, "%s,%s", tmpflds, data->key);
                        asprintf(&vals, "%2$s,%1$c%3$s%1$c",
                                quot, tmpvals, data->value);
                        free(tmpflds); free(tmpvals);
                }
                data = data->next;
        }

        asprintf(&sql,"INSERT INTO %s (%s) VALUES (%s)", resource, flds, vals);
        free(flds); free(vals);
        syslog(LOG_DEBUG, "%s", sql);

        if (db->conn == NULL) {
                if (db_connect(db) != 0) {
                        syslog(LOG_ERR, "Failed to connect to db on %s",
                                db->host);
                        return -1;
                }
                isconn = 1;
        }

        rval = db_exec_sql(db, sql);
        free(sql);

        /* leave the connection how we found it */
        if (isconn == 1)
                db_disconnect(db);

        return rval;
}

/* test credentials against ldap */
int db_test_bind(db_t *db, char *bindstr, char *bindattr,
        char *user, char *pass)
{
        char *binddn;
        int rc;

        if (db == NULL) {
                syslog(LOG_DEBUG, "NULL db passed to db_test_bind()");
                return HTTP_INTERNAL_SERVER_ERROR;
        }
        
        db_connect_ldap(db);
        asprintf(&binddn, "%s=%s,%s,%s", bindattr, user, bindstr, db->db);
        fprintf(stderr, "%s\n", binddn);
        rc = ldap_simple_bind_s(db->conn, binddn, pass);
        free(binddn);
        if (rc != LDAP_SUCCESS) {
                syslog(LOG_DEBUG, "Bind error: %s (%d)",
                        ldap_err2string(rc), rc);
                db_disconnect_ldap(db);
                return HTTP_UNAUTHORIZED;
        }
        db_disconnect_ldap(db);

        return 0;
}

/* free field_t struct */
void free_fields(field_t *f)
{
        field_t *next_f = NULL;
        while (f != NULL) {
                free(f->fname);
                free(f->fvalue);
                next_f = f->next;
                free(f);
                f = next_f;
        }
}

/* free row_t struct */
void liberate_rows(row_t *r)
{
        row_t *next_r = NULL;
        while (r != NULL) {
                free_fields(r->fields);
                next_r = r->next;
                free(r);
                r = next_r;
        }
}
