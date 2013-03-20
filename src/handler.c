/*
 * handler.c - some code to handle incoming connections
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

#define BUFSIZE 8096

#include "auth.h"
#include "config.h"
#include "db.h"
#include "handler.h"
#include "main.h"
#include "mime.h"
#include "xml.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <libxml/parser.h>
#include <limits.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>

int sockme;

http_status_code_t response_xslpost(int sock, url_t *u);
field_t *get_element(int *err);

/*
 * get sockaddr, IPv4 or IPv6:
 */
void *get_in_addr(struct sockaddr *sa)
{
        if (sa->sa_family == AF_INET) {
                return &(((struct sockaddr_in*)sa)->sin_addr);
        }

        return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/*
 * child process where we handle incoming connections
 */
void handle_connection(int sock, struct sockaddr_storage their_addr)
{
        char buf[BUFSIZE];
        char *mtype;
        char s[INET6_ADDRSTRLEN];
        http_status_code_t err;
        int auth = -1;
        int hcount = 0;
        int state;
        ssize_t byte_count;
        url_t *u;
        long len;

        inet_ntop(their_addr.ss_family,
                        get_in_addr((struct sockaddr *)&their_addr),
                        s, sizeof s);

        syslog(LOG_INFO, "server: got connection from %s", s);

        /* What are we being asked to do? */
        byte_count = recv(sock, buf, sizeof buf, 0);

        /* read http client request */
        request = http_read_request(buf, byte_count, &hcount, &err);
        if (err != 0) {
                http_response(sock, err);
                exit(EXIT_FAILURE);
        }
        
        /* keep a note of client ip */
        asprintf(&request->clientip, "%s", s);

        http_validate_headers(request, &err);
        if (err != 0) {
                syslog(LOG_INFO, "Bad Request - invalid request headers");
                http_response(sock, err);
                exit(EXIT_FAILURE);
        }

        syslog(LOG_DEBUG, "Client header count: %i", hcount);
        syslog(LOG_DEBUG, "Method: %s", request->method);
        syslog(LOG_DEBUG, "Resource: %s", request->res);
        syslog(LOG_DEBUG, "HTTP Version: %s", request->httpv);

        /* Return HTTP response */

        /* put a cork in it */
        state = 1;
        setsockopt(sockme, IPPROTO_TCP, TCP_CORK, &state, sizeof(state));

        /* if / requested, substitute default */
        if (strcmp(request->res, "/") == 0) {
                free(request->res);
                request->res = strdup(config->urldefault);
        }

        /* check auth & auth */
        auth = check_auth(request);
        if (auth != 0) {
                http_response(sock, auth);
                goto close_connection;
        }

        /* match url */
        u = http_match_url(request);
        if (u == NULL) {
                /* Not found */
                http_response(sock, HTTP_NOT_FOUND);
        }
        else {
                if (strcmp(request->method, "POST") == 0) {

                        /* FIXME: temp (begins) */
                        FILE *fd;
                        fd = fopen("/tmp/lastpost", "w");
                        fprintf(fd, "%s", buf);
                        fclose(fd);
                        /* FIXME: temp (ends) */

                        /* POST requires Content-Length header */
                        http_status_code_t err;

                        len = check_content_length(request, &err);
                        if (err != 0) {
                                http_response(sock, err);
                                goto close_connection;
                        }
                        else {
                                syslog(LOG_DEBUG, "Content-Length: %li", len);
                        }

                        mtype = check_content_type(request, &err);
                        if (err != 0) {
                                syslog(LOG_DEBUG,
                                        "Unsupported Media Type '%s'", mtype);
                                http_response(sock, err);
                                goto close_connection;
                        }
                }
                if (strncmp(u->type, "static", 6) == 0) {
                        /* serve static files */
                        err = response_static(sock, u);
                        if (err != 0)
                                http_response(sock, err);
                }
                else if (strcmp(u->type, "sqlview") == 0) {
                        /* handle sqlview */
                        err = response_sqlview(sock, u);
                        if (err != 0)
                                http_response(sock, err);
                }
                else if (strcmp(u->type, "xslpost") == 0) {
                        err = response_xslpost(sock, u);
                        if (err != 0)
                                http_response(sock, err);
                }
                else {
                        syslog(LOG_ERR, "Unknown url type '%s'", u->type);
                }
        }

close_connection:

        /* close client connection */
        close(sock);

        /* free memory */
        free_request(request);
        free_config();

        /* child process can exit */
        exit (EXIT_SUCCESS);
}

void respond (int fd, char *response)
{
        send(fd, response, strlen(response), 0);
}

/* handle sqlview */
http_status_code_t response_sqlview(int sock, url_t *u)
{
        char *r;
        char *sql;
        field_t *filter = NULL;
        char *xml;
        int err;
        db_t *db;

        if (!(db = getdb(u->db))) {
                syslog(LOG_ERR, "db '%s' not in config", u->db);
                return HTTP_INTERNAL_SERVER_ERROR;
        }
        
        /* fetch element id as filter, if applicable */
        filter = get_element(&err);
        if (err != 0) {
                return err;
        }

        if (strcmp(request->method, "POST") == 0) {
                if (filter == NULL) {
                        /* POST to collection => create */
                        /* TODO: validate field data */
                        if (db_insert(db, u->view, request->data) != 0) {
                                return HTTP_INTERNAL_SERVER_ERROR;
                        }
                }
                else {
                        /* POST to element => update */
                        /* TODO */
                }
        }

        if (asprintf(&sql, "%s", getsql(u->view)) == -1)
        {
                return HTTP_INTERNAL_SERVER_ERROR;
        }
        if (sqltoxml(db, sql, filter, &xml, 1) != 0) {
                free(sql);
                return HTTP_INTERNAL_SERVER_ERROR;
        }
        free(sql);
        if (asprintf(&r, RESPONSE_200, MIME_XML, xml) == -1)
        {
                free(xml);
                return HTTP_INTERNAL_SERVER_ERROR;
        }
        respond(sock, r);
        free(r);
        free(xml);

        return 0;
}

/* handle xslpost */
http_status_code_t response_xslpost(int sock, url_t *u)
{
        db_t *db;
        char *xsd;
        char *xsl;
        char *sql;
        char *action;
        int err;
        field_t *filter = NULL;

        if (strcmp(u->method, "POST") != 0) {
                syslog(LOG_ERR, "xslpost method not POST");
                return HTTP_METHOD_NOT_ALLOWED;
        }
        
        if (!(db = getdb(u->db))) {
                syslog(LOG_ERR, "db '%s' not in config", u->db);
                return HTTP_INTERNAL_SERVER_ERROR;
        }

        /* fetch element id as filter, if applicable */
        filter = get_element(&err);
        if (err != 0) {
                return err;
        }

        if (filter == NULL) {
                /* POST to collection => create */
                asprintf(&action, "create");
        }
        else {
                /* POST to element => update */
                asprintf(&action, "update");
        }

        /* validate request body xml */
        assert(asprintf(&xsd, "%s/%s/%s.xsd", config->xmlpath, u->view, action)
                != -1);
        if (xml_validate(xsd, request->data->value) != 0) {
                free(xsd);
                syslog(LOG_DEBUG, "%s", request->data->value);
                syslog(LOG_ERR, "Request XML failed validation");
                return HTTP_BAD_REQUEST;
        }
        free(xsd);

        /* transform xml into sql */
        assert(asprintf(&xsl, "%s/%s/%s.xsl", config->xmlpath, u->view, action)
                != -1);

        free(action);

        if (xmltransform(xsl, request->data->value, &sql) != 0) {
                free(xsl);
                syslog(LOG_ERR, "XSLT transform failed");
                return HTTP_BAD_REQUEST;
        }
        free(xsl);

        /* execute sql */
        if (db_exec_sql(db, sql) != 0) {
                free(sql);
                syslog(LOG_ERR, "xsltpost sql execution failed");
                return HTTP_BAD_REQUEST;
        }
        free(sql);

        respond(sock,
            "<response><code>200</code><message>Success</message></response>");

        return 0;
}

/* serve static files */
http_status_code_t response_static(int sock, url_t *u)
{
        char *filename;
        char *basefile;
        http_status_code_t err = 0;

        basefile = strdup(request->res + strlen(u->url) - 1);

        syslog(LOG_DEBUG, "basefile: %s", basefile);
        syslog(LOG_DEBUG, "u->path: %s", u->path);

        asprintf(&filename, "%s%s", u->path, basefile);
        free(basefile);
        send_file(sock, filename, &err);
        free(filename);

        return err;
}

/* send a static file */
int send_file(int sock, char *path, http_status_code_t *err)
{
        char *r;
        char *mimetype;
        int f;
        int rc;
        off_t offset;
        int state;
        struct stat stat_buf;

        *err = 0;

        f = open(path, O_RDONLY);
        if (f == -1) {
                syslog(LOG_ERR, "unable to open '%s': %s\n", path,
                                strerror(errno));
                *err = HTTP_NOT_FOUND;
                return -1;
        }

        /* get size of file */
        fstat(f, &stat_buf);

        /* ensure file is a regular file */
        if (! S_ISREG(stat_buf.st_mode)) {
                syslog(LOG_ERR, "'%s' is not a regular file\n", path);
                *err = HTTP_NOT_FOUND;
                return -1;
        }

        syslog(LOG_DEBUG, "Sending %i bytes", (int)stat_buf.st_size);

        /* send headers */
        mimetype = malloc(strlen(MIME_DEFAULT)+1);
        get_mime_type(mimetype, path);
        syslog(LOG_DEBUG, "Content-Type: %s", mimetype);
        if (asprintf(&r, RESPONSE_200, mimetype, "") == -1) {
                syslog(LOG_ERR, "send_file(): malloc failed");
                *err = HTTP_INTERNAL_SERVER_ERROR;
                return -1;
        }
        free(mimetype);
        respond(sock, r);

        /* send the file */
        errno = 0;
        offset = 0;
        rc = sendfile(sock, f, &offset, stat_buf.st_size);
        if (rc == -1) {
                syslog(LOG_ERR, "error from sendfile: %s\n", strerror(errno));
        }
        syslog(LOG_DEBUG, "Sent %i bytes", rc);

        /* everything sent ? */
        if (rc != stat_buf.st_size) {
                syslog(LOG_ERR,
                        "incomplete transfer from sendfile: %d of %d bytes\n",
                        rc, (int)stat_buf.st_size);
        }

        /* pop my cork */
        state = 0;
        setsockopt(sockme, IPPROTO_TCP, TCP_CORK, &state, sizeof(state));

        close(f);

        return 0;
}

/* if not a collection, fetch the last element from the request url */
field_t * get_element(int *err) {
        int i;
        field_t * filter = NULL;

        *err = 0;

        if (strncmp(request->res + strlen(request->res) - 1, "/", 1) != 0) {
                /* url didn't end in / - this is an element of a collection */
                filter = malloc(sizeof(field_t));
                if (asprintf(&filter->fname, "id") == -1) {
                        *err = HTTP_INTERNAL_SERVER_ERROR;
                        return NULL;
                }
                /* grab the key (the last segment of the url) */
                for (i = strlen(request->res); i > 0; --i) {
                        if (strncmp(request->res + i, "/", 1) == 0)
                                break;
                }
                if (i == 0) {
                        *err = HTTP_INTERNAL_SERVER_ERROR;
                        return NULL;
                }
                filter->fvalue = strdup(request->res + i + 1);
                syslog(LOG_DEBUG, "URL requested: %s", request->res);
                syslog(LOG_DEBUG, "Element id: %s", filter->fvalue);
        }
        return filter;
}
