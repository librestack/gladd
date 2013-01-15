/* 
 * config.h
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

#ifndef __GLADD_CONFIG_H__
#define __GLADD_CONFIG_H__ 1

#include <stdio.h>

#define BACKLOG 10  /* how many pending connectiong to hold in queue */
#define BUFSIZE 8096
#define LOCKFILE ".gladd.lock"
#define PROGRAM "gladd"
#define DEFAULT_CONFIG "/etc/gladd.conf"

extern int sockme;

typedef struct acl_t {
        char *type; /* allow or deny */
        char *method;
        char *url;
        char *auth;
        char *params;
        struct acl_t *next;
} acl_t;

typedef struct config_t {
        long debug;
        long port;
        char *authrealm;
        struct url_t *urls;
        struct acl_t *acls;
        struct db_t *dbs;
} config_t;

typedef struct db_t {
        char *alias; /* a convenient handle to refer to this db by */
        char *type;  /* "pg" = postgres only supported at present */
        char *host;  /* hostname or ip for this database eg. "localhost" */
        char *db;    /* name of the database */
        void *conn;  /* pointer to open db connection */
        struct db_t *next; /* pointer to next db so we can loop through them */
} db_t;

typedef struct url_t {
        char *type;
        char *url;
        char *path;
        char *db;
        char *view;
        struct url_t *next;
} url_t;

extern config_t *config;

FILE *open_config(char *configfile);
int process_config_line(char *line);
int read_config(char *configfile);
int set_config_defaults();
int set_config_long(long *confset, char *keyname, long i, long min, long max);
int add_acl (char *value);
int add_db (char *value);
void free_acls();
void free_dbs();
void free_urls();

#endif /* __GLADD_CONFIG_H__ */
