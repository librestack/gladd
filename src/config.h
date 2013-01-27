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

typedef struct auth_t {
        char *alias;
        char *type;
        char *db;
        char *sql;
        char *bind;     /* field to bind as */
        struct auth_t *next;
} auth_t;

typedef struct config_t {
        char *authrealm;
        long daemon;         /* 0 = daemonise (default), 1 = don't detach */
        long debug;
        char *encoding;      /* encoding to use - default UTF-8 */
        long port;
        struct acl_t *acls;
        struct auth_t *auth;
        struct db_t *dbs;
        struct sql_t *sql;
        struct url_t *urls;
} config_t;

typedef struct db_t {
        char *alias; /* a convenient handle to refer to this db by */
        char *type;  /* "pg" = postgres only supported at present */
        char *host;  /* hostname or ip for this database eg. "localhost" */
        char *db;    /* name of the database */
        char *user;  /* username (mysql) */
        char *pass;  /* password (mysql) */
        void *conn;  /* pointer to open db connection */
        struct db_t *next; /* pointer to next db so we can loop through them */
} db_t;

typedef struct keyval_t {
        char *key;
        char *value;
        struct keyval_t *next;
} keyval_t;

typedef struct sql_t {
        char *alias;
        char *sql;
        struct sql_t *next;
} sql_t;

typedef struct url_t {
        char *type;
        char *url;
        char *path;
        char *db;
        char *view;
        struct url_t *next;
} url_t;

extern config_t *config;
extern int g_signal;

int     add_acl (char *value);
int     add_auth (char *value);
int     add_db (char *value);
int     add_sql (char *value);
int     add_url_handler(char *value);
void    free_acls();
void    free_auth();
void    free_config();
void    free_dbs();
void    free_keyval(keyval_t *h);
void    free_sql();
void    free_urls();
auth_t *getauth(char *alias);
db_t   *getdb(char *alias);
char   *getsql(char *alias);
FILE   *open_config(char *configfile);
int     process_config_line(char *line);
int     read_config(char *configfile);
int     set_config_defaults();
int     set_config_long(long *confset, char *keyname, long i, long min,
                long max);
int     set_encoding(char *value);

#endif /* __GLADD_CONFIG_H__ */
