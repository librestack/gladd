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

#include "auth.h"
#include "config.h"
#include "gladdb/db.h"
#include "handler.h"
#include "main.h"
#include "mime.h"
#include "string.h"
#include "xml.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <libxml/parser.h>
#include <limits.h>
#include <netinet/tcp.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
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
        char *mtype;
        char s[INET6_ADDRSTRLEN];
        http_status_code_t err;
        int auth = -1;
        int hcount = 0;
        int state;
        url_t *u;
        long len;

        inet_ntop(their_addr.ss_family,
                        get_in_addr((struct sockaddr *)&their_addr),
                        s, sizeof s);

        syslog(LOG_DEBUG, "server: got connection from %s", s);

        /* read http client request */
        request = http_read_request(sock, &hcount, &err);
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

        /* X-Forwarded-For */
        if (config->xforward == 1) {
                char *xforwardip;
                xforwardip = http_get_header(request, "X-Forwarded-For");
                if (xforwardip) {
                        syslog(LOG_DEBUG, "X-Forwarded-For: %s", xforwardip);
                        request->xforwardip = strdup(xforwardip);
                }
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
                syslog(LOG_DEBUG, "failed to find matching url in config");
                http_response(sock, HTTP_NOT_FOUND);
        }
        else {
                if (strcmp(request->method, "POST") == 0) {
                        /* POST requires Content-Length header */
                        http_status_code_t err;

                        len = check_content_length(request, &err);
                        if (err != 0) {
                                syslog(LOG_DEBUG, "Incorrect content length");
                                http_response(sock, err);
                                goto close_connection;
                        }
                        else {
                                syslog(LOG_DEBUG, "Content-Length: %li", len);
                        }

                        mtype = check_content_type(request, &err, u->type);
                        if (err != 0) {
                                syslog(LOG_ERR,
                                        "Unsupported Media Type '%s'", mtype);
                                http_response(sock, err);
                                goto close_connection;
                        }
                }
                syslog(LOG_DEBUG, "Type: %s", u->type);
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
                else if (strcmp(u->type, "xslt") == 0) {
                        err = response_xslt(sock, u);
                        if (err != 0)
                                http_response(sock, err);
                }
                else if (strcmp(u->type, "upload") == 0) {
                        err = response_upload(sock, u);
                        if (err != 0)
                                http_response(sock, err);
                }
                else if (strcmp(u->type, "plugin") == 0) {
                        err = response_plugin(sock, u);
                        if (err != 0)
                                http_response(sock, err);
                }
                else if ((strcmp(u->type, "proxy") == 0)
                || (strcmp(u->type, "rewrite") == 0))
                {
                        err = http_response_proxy(sock, u);
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
        char *xml;
        char *xsd;
        char *xsl;
        char *sql;
        char *r;
        char *action;
        int err;
        field_t *filter = NULL;

        syslog(LOG_DEBUG, "response_xslpost()");

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

        syslog(LOG_DEBUG, "Performing XSLT Transformation");

        if (xmltransform(xsl, request->data->value, &sql) != 0) {
                free(xsl);
                syslog(LOG_ERR, "XSLT transform failed");
                return HTTP_INTERNAL_SERVER_ERROR;
        }
        free(xsl);

        /* connect to database */
        db_connect(db);

        syslog(LOG_DEBUG, "Executing SQL");

        /* execute sql */
        if (db_exec_sql(db, sql) != 0) {
                free(sql);
                syslog(LOG_ERR, "xsltpost sql execution failed");
                return HTTP_INTERNAL_SERVER_ERROR;
        }
        free(sql);

        /* check if we have results to return (created.xsl / updated.xsl) */
        assert(asprintf(&xsl, "%s/%s/%sd.xsl", config->xmlpath, u->view,
                action) != -1);
        free(action);
        if (access(xsl, R_OK) != 0) {
                /* No xsl for result, just return OK */
                http_response(sock, HTTP_OK);
        }
        else {
                /* return row(s) as result */
                if (xmltransform(xsl, request->data->value, &sql) != 0) {
                        free(xsl);
                        syslog(LOG_ERR, "XSLT transform failed");
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
        }
        free(xsl);
        db_disconnect(db);

        return 0;
}

http_status_code_t response_xslt(int sock, url_t *u)
{
        char *r;
        char *sql;
        char *xml;
        char *xsl;
        char *html;
        int err;
        db_t *db;
        field_t *filter = NULL;

        syslog(LOG_DEBUG, "response_xslt()");

        /* ensure method is GET */
        if (strcmp(u->method, "GET") != 0) {
                syslog(LOG_ERR, "xslt method not GET");
                return HTTP_METHOD_NOT_ALLOWED;
        }
       
        /* ensure we have a valid database */
        if (!(db = getdb(u->db))) {
                syslog(LOG_ERR, "db '%s' not in config", u->db);
                return HTTP_INTERNAL_SERVER_ERROR;
        }

        /* ensure we have some sql to work with */
        if (!(sql = getsql(u->view))) {
                syslog(LOG_ERR, "sql '%s' not in config", u->view);
                return HTTP_INTERNAL_SERVER_ERROR;
        }

        /* fetch element id as filter, if applicable */
        filter = get_element(&err);
        if (err != 0) {
                return err;
        }

        /* fetch data as xml */
        if (sqltoxml(db, sql, filter, &xml, 1) != 0) {
                free(sql);
                return HTTP_INTERNAL_SERVER_ERROR;
        }

        /* ensure XSLT file exists */
        assert(asprintf(&xsl, "%s/%s/view.xsl", config->xmlpath, u->view)
                != -1);

        /* transform xml data into html */
        syslog(LOG_DEBUG, "Performing XSLT Transformation");
        if (xmltransform(xsl, xml, &html) != 0) {
                free(xsl); free(xml);
                syslog(LOG_ERR, "XSLT transform failed");
                return HTTP_BAD_REQUEST;
        }
        free(xsl); free(xml);

        /* build response */
        if (asprintf(&r, RESPONSE_200, MIME_HTML, html) == -1)
        {
                free(html);
                return HTTP_INTERNAL_SERVER_ERROR;
        }

        /* return html response */
        respond(sock, r);
        free(r);
        free(html);

        return 0;
}

/* receive uploaded files */
http_status_code_t response_upload(int sock, url_t *u)
{
        EVP_MD_CTX *mdctx;
        char **tokens;
        char *b = request->boundary;
        char *clen;
        char *filename;
        char *pbuf;
        char *ptmp;
        char *url;
        char hash[SHA_DIGEST_LENGTH*2];
        char template[] = "/var/tmp/upload-XXXXXX";
        const EVP_MD *md;
        http_status_code_t err = 0;
        int fd;
        int toknum;
        long lclen;
        size_t size = 0;
        struct timeval tv;
        unsigned char md_value[EVP_MAX_MD_SIZE];
        unsigned int md_len, i;

        /* get expected length of body */
        clen = http_get_header(request, "Content-Length");
        lclen = strtol(clen, NULL, 10);

        /* first, grab any bytes already in buffer */
        size = bytes;
        syslog(LOG_DEBUG, "I already have %i bytes", (int) size);

        /* find boundary */
        pbuf = memsearch(buf, b, BUFSIZE);
        if (pbuf == NULL) {
                syslog(LOG_ERR, "No boundary found in data");
                return HTTP_BAD_REQUEST;
        }
        pbuf += strlen(b) + 2; /* skip boundary and CRLF */

        /* skip headers => find blank line (search for 2xCRLF) */
        pbuf = memsearch(pbuf, "\r\n\r\n", BUFSIZE-(pbuf-buf));
        if (pbuf == NULL) {
                syslog(LOG_ERR, "Blank line required after multipart headers");
                return HTTP_BAD_REQUEST;
        }
        pbuf += 4;

        /* open file for writing */
        fd = mkstemp(template);

        /* start building SHA1 */
        OpenSSL_add_all_digests();
        md = EVP_get_digestbyname("SHA1");
        if(!md) {
                syslog(LOG_ERR, "SHA1 digest unavailable");
                return HTTP_INTERNAL_SERVER_ERROR;
        }
        mdctx = EVP_MD_CTX_create();
        EVP_DigestInit_ex(mdctx, md, NULL);

        ptmp = memsearch(pbuf, b, BUFSIZE-(pbuf-buf)); /* check for boundary */
        if (ptmp != NULL) {
                /* boundary found - short write */
                syslog(LOG_DEBUG, "end boundary found in initial buffer");
                write(fd, pbuf, ptmp - pbuf - 4);
                EVP_DigestUpdate(mdctx, pbuf, ptmp - pbuf - 4);
        }
        else {
                syslog(LOG_DEBUG, "end boundary NOT found in initial buffer");
                write(fd, pbuf, BUFSIZE - (pbuf - buf) - 2);
                EVP_DigestUpdate(mdctx, pbuf, BUFSIZE - (pbuf - buf) - 2);
        }

        if (lclen > sizeof buf) { 
                http_flush_buffer();
                bytes = sizeof buf;

                /* set socket timeout */
                tv.tv_sec = 1;
                tv.tv_usec = 0;
                setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
                        (char *)&tv, sizeof(struct timeval));
                while (bytes > 0) {
                        /* read into buffer */
                        errno = 0;
                        bytes = recv(sock, buf, BUFSIZE, MSG_WAITALL);
                        if (bytes == -1) {
                                syslog(LOG_ERR,"Error reading from socket: %s",
                                        strerror(errno));
                                        
                        }
                        if (bytes == 0) break; /* no bytes read */
                        size += bytes;
                        syslog(LOG_DEBUG, "Read %i bytes", (int)bytes);

                        /* check for boundary */
                        pbuf = memsearch(buf, b, bytes);
                        if (pbuf != NULL) {
                                /* boundary reached, we're done here */
                                syslog(LOG_DEBUG, "boundary reached, we're done here");
                                write(fd, buf, pbuf - buf - 4);
                                EVP_DigestUpdate(mdctx, buf, pbuf - buf - 4);
                                break;
                        }

                        /* write contents of buffer out to file */
                        write(fd, buf, (int) bytes);
                        EVP_DigestUpdate(mdctx, buf, bytes);
                }
        }

        if (lclen != size) {
                syslog(LOG_ERR, "ERROR: Read %li/%li bytes",
                        (long) size, lclen);
                syslog(LOG_ERR, "ERROR: Expected another %li bytes",
                        lclen - (long)size);
                return HTTP_BAD_REQUEST;
        }

        syslog(LOG_DEBUG, "Read %li bytes total", (long)size);

        EVP_DigestFinal_ex(mdctx, md_value, &md_len);
        EVP_MD_CTX_destroy(mdctx);

        for (i=0; i < SHA_DIGEST_LENGTH; i++) {
                sprintf((char*)&(hash[i*2]), "%02x", md_value[i]);
        }

        syslog(LOG_DEBUG, "hash: %s", hash);

        /* TODO: deal with other data parts */

        /* set permissions */
        if (fchmod(fd, 0644) == -1) {
                syslog(LOG_ERR, "Failed to set file permissions on upload");
                return HTTP_INTERNAL_SERVER_ERROR;
        }

        /* close file */
        close(fd);

        /* ensure destination directory exists */
        url = strdup(request->res);
        tokens = tokenize(&toknum, &url, "/");
        free(url);
        umask(022);
        asprintf(&filename, "%s/%s", u->path, tokens[2] );
        if (mkdir(filename, 0755) != 0) {
                if (errno != EEXIST) {
                        syslog(LOG_ERR, "Error creating directory '%s': %s",
                                filename, strerror(errno));
                        free(tokens);
                        free(filename);
                        return HTTP_INTERNAL_SERVER_ERROR;
                }
        }
        free(filename);

        /* rename to <path>/<instance>/<sha1sum> */
        asprintf(&filename, "%s/%s/%s", u->path, tokens[2], hash);
        free(tokens);
        syslog(LOG_ERR, "filename: %s", filename);
        if (rename(template, filename) == -1) {
                syslog(LOG_ERR, "Failed to rename uploaded file: %s",
                        strerror(errno));
                free(filename);
                return HTTP_INTERNAL_SERVER_ERROR;
        }
        free(filename);

        /* return hash of uploaded file */
        respond(sock, hash);

        return err;
}

/* call a plugin */
http_status_code_t response_plugin(int sock, url_t *u)
{
        FILE *fd;
        char pbuf[BUFSIZE] = "";
        http_status_code_t err = 0;
        int ret;
        ssize_t ibytes = BUFSIZE;
        char *cmd;
        int state;

        cmd = strdup(u->path);
        sqlvars(&cmd, request->res);
        syslog(LOG_DEBUG, "executing plugin: %s", cmd);
        fd = popen(cmd, "r");
        if (fd == NULL) {
                syslog(LOG_ERR, "popen(): %s", strerror(errno));
                return HTTP_INTERNAL_SERVER_ERROR;
        }
        free(cmd);

        /* TODO: write data to plugin */

        /* TODO: write HTTP headers */

        /* keep reading from plugin and sending output back to HTTP client */
        while (ibytes == BUFSIZE) {
                ibytes = fread(pbuf, 1, BUFSIZE, fd);
                syslog(LOG_DEBUG, "writing %i bytes to socket", (int) ibytes);
                send(sock, pbuf, ibytes, 0);
        }

        /* pop TCP cork */
        state = 0;
        setsockopt(sock, IPPROTO_TCP, TCP_CORK, &state, sizeof(state));

        /* TODO: handle exit codes */
        ret = pclose(fd);
        if (ret == -1) {
                syslog(LOG_ERR, "pclose(): %s", strerror(errno));
                //return HTTP_INTERNAL_SERVER_ERROR;
        }


        return err;
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
