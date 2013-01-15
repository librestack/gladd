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
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include "db.h"

int db_connect_pg(db_t *db)
{
        char *conninfo;
        PGconn     *conn;

        if (asprintf(&conninfo,
                "host = %s dbname = %s", db->host, db->db) == -1)
        {
                fprintf(stderr, 
                        "asprintf failed for conninfo in db_connect_pg()\n");
                return -2;
        }
        conn = PQconnectdb(conninfo);
        if (PQstatus(conn) != CONNECTION_OK)
        {
                syslog(LOG_ERR, "connection to database failed.\n");
                syslog(LOG_DEBUG, "%s", PQerrorMessage(conn));
                fprintf(stderr, "%s", PQerrorMessage(conn));
                return PQstatus(conn);
        }
        db->conn = conn;
        return 0;
}

/* connect to specified database 
 * pointer to the connection is stored in db_t struct 
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
        else {
                fprintf(stderr, 
                        "Invalid database type '%s' passed to db_connect()\n", 
                        db->type);
        }
        return 0;
}

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
        else {
                fprintf(stderr, 
                    "Invalid database type '%s' passed to db_disconnect()\n",
                    db->type);
        }
        return 0;
}

int db_disconnect_pg(db_t *db)
{
        /*
        if (sizeof(db->conn) != sizeof(PGconn)) {
                fprintf(stderr, "db_disconnect_pg() called on non-PG db\n");
                return;
        }
        */
        PQfinish(db->conn);

        return 0;
}

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

int db_create_pg(db_t *db)
{
        
        return 0;
}
