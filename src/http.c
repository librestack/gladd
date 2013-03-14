/* 
 * http.c
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
#include <assert.h>
#include <b64/cdecode.h>
#include <curl/curl.h>
#include <errno.h>
#include <fnmatch.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include "handler.h"
#include "http.h"
#include "string.h"

/* 
 * HTTP response codes from:
 *   http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
 */
struct http_status httpcode[] = {
        { 200, "OK" },
        { 201, "Created" },
        { 202, "Accepted" },
        { 203, "Non-Authoritative Information" },
        { 204, "No Content" },
        { 205, "Reset Content" },
        { 206, "Partial Content" },
        { 300, "Multiple Choices" },
        { 301, "Moved Permanently" },
        { 302, "Found" },
        { 303, "See Other" },
        { 304, "Not Modified" },
        { 305, "Use Proxy" },
        { 307, "Temporary Redirect" },
        { 400, "Bad Request" },
        { 401, "Unauthorized" },
        { 402, "Payment Required" },
        { 403, "Forbidden" },
        { 404, "Not Found" },
        { 405, "Method Not Allowed" },
        { 406, "Not Acceptable" },
        { 407, "Proxy Authentication Required" },
        { 408, "Request Timeout" },
        { 409, "Conflict" },
        { 410, "Gone" },
        { 411, "Length Required" },
        { 412, "Precondition Failed" },
        { 413, "Request Entity Too Large" },
        { 414, "Request-URI Too Long" },
        { 415, "Unsupported Media Type" },
        { 416, "Requested Range Not Satisfiable" },
        { 417, "Expectation Failed" },
        { 500, "Internal Server Error" },
        { 505, "HTTP Version Not Supported" }
};

http_request_t *request;

/* 
 * bodyline() - Also known as Fast Leg Theory.
 * handles each line of the body of the request.
 * Remember to duck.
 */
void bodyline(http_request_t *r, char *line)
{
        CURL *handle;
        char *clear;
        char *despaced;
        char dtok[LINE_MAX] = "";
        char key[LINE_MAX] = "";
        char value[LINE_MAX] = "";
        int l = 0;
        FILE *fd;

        handle = curl_easy_init();

        fd = fmemopen(line, strlen(line), "r");

        while (fscanf(fd, "%[^&]&", dtok) == 1) {
                /* curl_easy_unescape() has a bug and doesn't unescape 
                 * '+' to space.
                 * see https://github.com/bmuller/mod_auth_openid/issues/10 */
                despaced = replaceall(dtok, "+", " "); /* fix curl bug */
                clear = curl_easy_unescape(handle, despaced, strlen(dtok), &l);
                if (sscanf(clear, "%[^=]=%[^\n]", key, value) == 2) {
                        http_add_request_data(r, key, value);
                        fprintf(stderr, "%s says %s\n", key, value);
                }
                free(clear);
                free(despaced);
        }

        fclose(fd);

        curl_easy_cleanup(handle);
        curl_global_cleanup();
}

/* Check we received a valid Content-Length header
 * returns either the length or -1 on error
 * err is set to the appropriate http status code on error
 */
int check_content_length(http_request_t *r, http_status_code_t *err)
{
        long len;
        char *clength;

        clength = http_get_header(r, "Content-Length");
        if (clength == NULL) {
                /* 411 - Length Required */
                *err = HTTP_LENGTH_REQUIRED;
                return -1;
        }
        errno = 0;
        len = strtol(clength, NULL, 10);
        if ((errno == ERANGE && (len == LONG_MAX || len == LONG_MIN))
                || (errno != 0 && len == 0))
        {
                /* Invalid length - return 400 Bad Request */
                *err = HTTP_BAD_REQUEST;
                return -1;
        }

        return len;
}

/* ensure Content-Type header is present and valid
 * returns content-type string or NULL on error
 * err is set to the appropriate http status code on error
 */
char *check_content_type(http_request_t *r, http_status_code_t *err)
{
        char *mtype;

        mtype = http_get_header(request, "Content-Type");
        if ((strcmp(mtype, "application/x-www-form-urlencoded") == 0)
        || (strncmp(mtype, "text/xml", 8) == 0))
        {
                return mtype;
        }
        else {
                syslog(LOG_DEBUG,
                        "Unsupported Media Type '%s'", mtype);
                *err = HTTP_UNSUPPORTED_MEDIA_TYPE;
                return NULL;
        }
}

/* return decoded base64 string */
char *decode64(char *str)
{
        int r;
        char *plain = NULL;

        plain = malloc(sizeof(str) * 2);

        base64_decodestate *d;
        d = malloc(sizeof(base64_decodestate));
        base64_init_decodestate(d);

        r = base64_decode_block(str, strlen(str), plain, d);

        free(d);

        return plain;
}

/* key -> value lookup for client request headers */
char *http_get_header(http_request_t *r, char *key)
{
        keyval_t *h = r->headers;
        while (h != NULL) {
                if (strcmp(h->key, key) == 0)
                        return h->value;
                h = h->next;
        }
        return NULL;
}

/* match requested url to config->urls */
url_t *http_match_url(http_request_t *r)
{
        url_t *u;

        syslog(LOG_DEBUG, "Searching for %s %s\n", r->method, r->res);
        u = config->urls;
        while (u != NULL) {
                syslog(LOG_DEBUG, "%s %s\n", u->method, u->url);
                if ((fnmatch(u->url, r->res, 0) == 0) &&
                         (strcmp(r->method, u->method) == 0))
                {
                        break;
                }
                u = u->next;
        }

        return u;
}


/* record key=value pair from client request */
void http_add_request_data(http_request_t *r, char *key, char *value)
{
        keyval_t *h;
        static keyval_t *hlast;
        
        h = http_set_keyval(key, value);

        if (r->data == NULL)
                r->data = h;
        else
                hlast->next = h;
        hlast = h;
}

/* return an initialized http request struct */
http_request_t *http_init_request()
{
        http_request_t *r;

        r = malloc(sizeof(http_request_t));
        r->httpv = NULL;
        r->method = NULL;
        r->res = NULL;
        r->querystr = NULL;
        r->clientip = NULL;
        r->authtype = NULL;
        r->authuser = NULL;
        r->authpass = NULL;
        r->headers = NULL;
        r->data = NULL;

        asprintf(&r->authtype,"Basic");
        
        return r;
}

/* return a keyval_t with the key and value set */
keyval_t *http_set_keyval (char *key, char *value)
{
        keyval_t *h;

        h = malloc(sizeof(keyval_t));
        h->key = strdup(key);
        h->value = strdup(value);
        h->next = NULL;

        return h;
}

/* set http_request_t->method to new value, reallocating memory if needed */
void http_set_request_method(http_request_t *r, char *method)
{
        int i;

        if (r->method != NULL) {
                free(r->method);
        }
        i = asprintf(&r->method, "%s", method);
        assert(i != -1);
                
}

/* set http_request_t->res to new value, reallocating memory if needed */
void http_set_request_resource(http_request_t *r, char *res)
{
        int i;
        char url[LINE_MAX];
        char qstr[LINE_MAX];

        if (r->res != NULL) {
                free(r->res);
        }
        if (sscanf(res, "%[^?]?%[^\n]", url, qstr) == 2) {
                /* we have a querystring - store it */
                i = asprintf(&r->res, "%s", url);
                assert(i != -1);
                i = asprintf(&r->querystr, "%s", qstr);
        }
        else {
                i = asprintf(&r->res, "%s", res);
        }
        assert(i != -1);
                
}

/* check & store http headers from client */
http_request_t *http_read_request(char *buf, ssize_t bytes, int *hcount,
        http_status_code_t *err)
{
        http_request_t *r;
        char *ctype;
        char *clen;
        long lclen;
        size_t size;
        char line[LINE_MAX] = "";
        char key[256] = "";
        char value[256] = "";
        keyval_t *h = NULL;
        keyval_t *hlast = NULL;
        char method[16] = "";
        char resource[MAX_RESOURCE_LEN] = "";
        char httpv[16] = "";
        char *stripped;
        FILE *in;
        char *xmlbuf;

        *err = 0;

        r = http_init_request();
        r->bytes = bytes;

        in = fmemopen(buf, strlen(buf), "r");
        assert (in != NULL);

        /* first line has http request */
        if (fgets(line, sizeof(line), in) == NULL) {
                *err = HTTP_BAD_REQUEST;
                return NULL;
        }
        if (sscanf(line, "%s %s HTTP/%s", method, resource, httpv) != 3) {
                *err = HTTP_BAD_REQUEST;
                return NULL;
        }
        r->httpv = strdup(httpv);

        if ((strcmp(r->httpv, "1.0") != 0) && (strcmp(r->httpv, "1.1") != 0)) {
                *err = HTTP_VERSION_NOT_SUPPORTED;
                return NULL;
        }

        http_set_request_method(r, method);
        http_set_request_resource(r, resource);

        /* read headers */
        while (fgets(line, sizeof(line), in)) {
                /* we strip out any \r which jquery puts in, before matching */
                stripped = replaceall(line, "\r", "");
                if (sscanf(stripped, "%[^:]: %[^\n]",
                        key, value) != 2)
                {
                        free(stripped);
                        break;
                }
                free(stripped);
                h = http_set_keyval(key, value);
                if (hlast != NULL) {
                        hlast->next = h;
                }
                else {
                        r->headers = h;
                }
                hlast = h;
                (*hcount)++;
        }

        /* only read body if we have a Content-Type and Content-Length */
        if ((ctype = http_get_header(r, "Content-Type"))
                && (clen = http_get_header(r, "Content-Length")))
        {
                errno = 0;
                lclen = strtol(clen, NULL, 10);
                if (errno != 0)
                        return NULL;
                if (strncmp(ctype, "text/xml", 8) == 0) {
                        /* read xml body */
                        r->data = malloc(sizeof(keyval_t));
                        asprintf(&r->data->key, "text/xml");
                        r->data->next = NULL;
                        xmlbuf = calloc(1, lclen + 1);
                        size = fread(xmlbuf, 1, lclen, in);
                        if (size != lclen) {
                                /* we have the wrong number of bytes */
                                *err = HTTP_BAD_REQUEST;
                                return NULL;
                        }
                        r->data->value = strdup(xmlbuf);
                        free(xmlbuf);
                }
                else {
                        /* read body, after skipping the blank line */
                        while (fgets(line, sizeof(line), in)) {
                                bodyline(r, line);
                        }
                }
        }

        fclose(in);

        return r;
}

/* Output http status code response to the socket */
void http_response(int sock, int code)
{
        char *response;
        char *status;
        char *mime = "text/html";
        char *headers = "";

        if (code == HTTP_UNAUTHORIZED) {
                /* 401 Unauthorized MUST include WWW-Authenticate header */
                asprintf(&headers, "\nWWW-Authenticate: %s realm=\"%s\"", 
                                request->authtype, config->authrealm);
        }

        status = get_status(code).status;
        asprintf(&response, HTTP_RESPONSE, code, status, mime, headers);
        respond(sock, response);
        free(response);
        free(headers);
}

/* find http status by numerical code */
struct http_status get_status(int code)
{
        int x;
        struct http_status hcode;

        for (x = 0; x < HTTPKEYS; x++) {
                if (httpcode[x].code == code)
                        hcode = httpcode[x];
        }

        return hcode;
}

/* process the headers from the client http request */
int http_validate_headers(http_request_t *r, http_status_code_t *err)
{
        char type[64] = "";
        char user[64] = "";
        char pass[64] = "";
        char cryptauth[128] = "";
        char *clearauth;
        keyval_t *h;

        *err = 0;

        h = r->headers;
        while (h != NULL) {
                if (strcmp(h->key, "Authorization") == 0) {
                        if (sscanf(h->value, "%s %s", type, cryptauth) == 2) {
                                clearauth = decode64(cryptauth);
                                if (sscanf(clearauth, "%[^:]:%[^:]",
                                        user, pass) == 2)
                                {
                                        free(r->authtype);
                                        r->authtype = strdup(type);
                                        r->authuser = strdup(user);
                                        r->authpass = strdup(pass);
                                        free(clearauth);
                                }
                                else {
                                        syslog(LOG_DEBUG,
                                                "Invalid auth details\n");
                                        free(clearauth);
                                        *err = HTTP_BAD_REQUEST;
                                        return -1;
                                }
                        }
                        else {
                                syslog(LOG_DEBUG,
                                        "Invalid Authorization header '%s'", h->value);
                                *err = HTTP_BAD_REQUEST;
                                return -1;
                        }
                }
                h = h->next;
        }
        return 0;
}

/* free request info */
void free_request(http_request_t *r)
{
        free_keyval(r->headers);
        free_keyval(r->data);
        free(r->httpv);
        free(r->method);
        free(r->res);
        free(r->querystr);
        free(r->clientip);
        free(r->authtype);
        free(r->authuser);
        free(r->authpass);
        free(r);
        r = NULL;
}
