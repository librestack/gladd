/* 
 * tds.c - code to handle database connections to TDS servers
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
#include "db.h"
#include "http.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sybdb.h>
#include <sybfront.h>
#include <syslog.h>

int err_handler(DBPROCESS*, int, int, int, char*, char*);
int msg_handler(DBPROCESS*, DBINT, int, int, char*, char*, char*, int);

/* connect to tds database */
int db_connect_tds(db_t *db)
{
        LOGINREC *login;
        DBPROCESS *dbproc;
        RETCODE erc;

        if (dbinit() == FAIL) {
                syslog(LOG_ERR, "dbinit() failed");
                return -1;
        }

        dberrhandle(err_handler);
        dbmsghandle(msg_handler);

        if ((login = dblogin()) == NULL) {
                syslog(LOG_ERR, "unable to allocate tds login structure");
                return -1;
        }

        DBSETLUSER(login, db->user);
        DBSETLPWD(login, db->pass);

        if ((dbproc = dbopen(login, db->host)) == NULL) {
                syslog(LOG_ERR, "unable to connect to %s as %s", db->host,
                        db->user);
                return -1;
        }

        if (db->db  && (erc = dbuse(dbproc, db->db)) == FAIL) {
                syslog(LOG_ERR, "unable to use to database %s", db->db);
                return -1;
        }

        db->conn = dbproc;

        return 0;
}

/* disconnect */
int db_disconnect_tds(db_t *db)
{
        //if (db->conn != NULL) dbclose(db->conn);
        dbexit();
        return 0;
}

int db_exec_sql_tds(db_t *db, char *sql)
{
        RETCODE erc;

        /* allocate memory for SQL */
        if ((erc = dbfcmd(db->conn, "%s ", sql)) == FAIL) {
                syslog(LOG_ERR, "dbcmd() failed");
                return -1;
        }

        /* ensure we don't have any results pending
         * - tds won't allow a connection to be reused until
         * all results have been processed */

        /* TODO: replace with cancel */
        int rc;
        while ((rc = dbresults(db->conn)) == SUCCEED) {
                while ((rc = dbnextrow(db->conn)) != NO_MORE_ROWS){
                }
        }

        /* execute query */
        if ((erc = dbsqlexec(db->conn)) == FAIL) {
                syslog(LOG_ERR, "dbsqlexec() failed");
                return -1;
        }

        return 0;
}

/* return all results from a SELECT - tds */
int db_fetch_all_tds(db_t *db, char *sql, field_t *filter, row_t **rows,
        int *rowc)
{

        DBPROCESS *dbproc = db->conn;
        RETCODE erc;
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

        if (db_exec_sql_tds(db, sql) != 0) {
                return -1;
        }

        /* fetch results */
        while ((erc = dbresults(dbproc)) != NO_MORE_RESULTS) {
                struct COL
                {
                        char *name;
                        char *buffer;
                        int type, size, status;
                } *columns, *pcol;

                int ncols;
                int row_code;

                if (erc == FAIL) {
                        fprintf(stderr, "dbresults failed\n");
                        return -1;
                }
                ncols = dbnumcols(dbproc);

                if ((columns = calloc(ncols, sizeof(struct COL))) == NULL) {
                        perror(NULL);
                        return -1;
                }

                /* Read metadata and bind. */
                for (pcol = columns; pcol - columns < ncols; pcol++) {
                        int c = pcol - columns + 1;

                        pcol->name = dbcolname(dbproc, c);
                        pcol->type = dbcoltype(dbproc, c);
                        pcol->size = dbcollen(dbproc, c);

                        if (SYBCHAR != pcol->type) {
                                pcol->size = dbwillconvert(pcol->type, SYBCHAR);
                        }

                        if ((pcol->buffer = calloc(1, pcol->size + 1)) == NULL){
                                perror(NULL);
                                return -1;
                        }

                        erc = dbbind(dbproc, c, NTBSTRINGBIND,
                        pcol->size+1, (BYTE*)pcol->buffer);
                        if (erc == FAIL) {
                                fprintf(stderr, "dbbind(%d) failed\n", c);
                                return -1;
                        }

                        erc = dbnullbind(dbproc, c, &pcol->status);

                        if (erc == FAIL) {
                                fprintf(stderr, "dbnullbind(%d) failed\n", c);
                                return -1;
                        }
                }
                while ((row_code = dbnextrow(dbproc)) != NO_MORE_ROWS){
                        switch (row_code) {
                                case REG_ROW:
                                        r = malloc(sizeof(row_t));
                                        r->fields = NULL;
                                        r->next = NULL;
                                        for (pcol=columns; pcol - columns < ncols; pcol++) {
                                                f = malloc(sizeof(field_t));
                                                f->fname = strdup(pcol->name);
                                                asprintf(&f->fvalue, "%s",
                                                  pcol->status == -1
                                                  ? "NULL" : pcol->buffer);
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
                                                /* as this is our first row,
                                                 * update the ptr */
                                                *rows = r;
                                        }
                                        else {
                                                rtmp->next = r;
                                        }
                                        ftmp = NULL;
                                        rtmp = r;
                                        break;

                                case BUF_FULL:
                                        assert(row_code != BUF_FULL);
                                        break;

                                case FAIL:
                                        fprintf(stderr, "dbresults failed\n");
                                        return -1;
                                        break;

                                default:
                                        printf("Data for computeid %d ignored\n", row_code);
                        }
                }

                assert(row_code != BUF_FULL);

                /* row count */
                if (DBCOUNT(dbproc) > -1)
                        fprintf(stderr, "%d rows affected\n", DBCOUNT(dbproc));

                /* check return status */
                if (dbhasretstat(dbproc) == TRUE) {
                        printf("Procedure returned %d\n", dbretstatus(dbproc));
                }
        }
        
        return 0;
}

int msg_handler(DBPROCESS *dbproc, DBINT msgno, int msgstate, int severity,
        char *msgtext, char *srvname, char *procname, int line)
{
        enum {changed_database = 5701, changed_language = 5703 };

        if (msgno == changed_database || msgno == changed_language)
                return 0;

        if (msgno > 0) {
                fprintf(stderr, "Msg %ld, Level %d, State %d\n",
                        (long) msgno, severity, msgstate);

                if (strlen(srvname) > 0)
                        fprintf(stderr, "Server '%s', ", srvname);
                if (strlen(procname) > 0)
                        fprintf(stderr, "Procedure '%s', ", procname);
                if (line > 0)
                        fprintf(stderr, "Line %d", line);

                fprintf(stderr, "\n\t");
        }
        fprintf(stderr, "%s\n", msgtext);

        if (severity > 10) {
                fprintf(stderr, "error: severity %d > 10, exiting\n",severity);
                return -1;
        }

        return 0;
}


int err_handler(DBPROCESS * dbproc, int severity, int dberr, int oserr,
                        char *dberrstr, char *oserrstr)
{
        if (dberr) {
                fprintf(stderr, "Msg %d, Level %d\n", dberr, severity);
                fprintf(stderr, "%s\n\n", dberrstr);
        }
        else {
                fprintf(stderr, "DB-LIBRARY error:\n\t");
                fprintf(stderr, "%s\n", dberrstr);
        }
        return INT_CANCEL;
}

