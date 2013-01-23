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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include "config.h"
#include "handler.h"
#include "http.h"

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

char *decode64(char *str)
{
        int r;
        char *plain;

        plain = malloc(sizeof(str));

        base64_decodestate *d;
        d = malloc(sizeof(base64_decodestate));
        base64_init_decodestate(d);

        r = base64_decode_block(str, strlen(str), plain, d);

        return plain;
}

char *http_get_header(http_header_t *h, char *key)
{
        while (h != NULL) {
                if (strcmp(h->key, key) == 0)
                        return h->value;
                h = h->next;
        }
        return NULL;
}

/* check & store http headers from client */
int http_read_headers(char *buf, ssize_t bytes)
{
        int c = 0;
        int hcount = 0;
        char key[256];
        char value[256];
        http_header_t *h;
        http_header_t *hlast = NULL;
        char m[16];
        char r[MAX_RESOURCE_LEN];
        char v[16];
        FILE *in;

        request = malloc(sizeof(http_request_t));
        request->bytes = bytes;
        request->authuser = NULL;
        request->authpass = NULL;
        request->headers = NULL;

        in = fmemopen(buf, strlen(buf), "r");
        assert (in != NULL);

        if (fscanf(in, "%s %s HTTP/%s", m, r, v) != 3) {
                /* Bad request */
                return -1;
        }
        request->httpv = strdup(v);
        request->method = strdup(m);
        request->res = strdup(r);

        for (;;) {
                c = fscanf(in, "%s %[^\n]", key, value);
                if (c <= 0)
                        break;
                h = malloc(sizeof(http_header_t));
                h->key = strdup(key);
                *(h->key + strlen(h->key) - 1) = '\0';
                h->value = strdup(value);
                h->next = NULL;
                if (hlast != NULL) {
                        hlast->next = h;
                }
                else {
                        request->headers = h;
                }
                hlast = h;
                hcount++;
        }

        fclose(in);

        return hcount;
}

void http_response(int sock, int code)
{
        char *response;
        char *status;
        char *mime = "text/html";
        char *headers = "";

        if (code == 401) {
                /* 401 Unauthorized MUST include WWW-Authenticate header */
                asprintf(&headers, "\nWWW-Authenticate: Basic realm=\"%s\"", 
                                config->authrealm);
        }

        status = get_status(code).status;
        asprintf(&response, HTTP_RESPONSE, code, status, mime, headers);
        respond(sock, response);
        free(response);
        free(headers);
}

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
int http_validate_headers(http_header_t *h)
{
        char user[64] = "";
        char pass[64] = "";
        char cryptauth[128] = "";
        char *clearauth;

        while (h != NULL) {
                if (strcmp(h->key, "Authorization") == 0) {
                        if (sscanf(h->value, "Basic %s", cryptauth) == 1) {
                                clearauth = decode64(cryptauth);
                                if (sscanf(clearauth, "%[^:]:%[^:]",
                                        user, pass) == 2)
                                {
                                        request->authuser = strdup(user);
                                        request->authpass = strdup(pass);
                                        free(clearauth);
                                }
                                else {
                                        fprintf(stderr,
                                                "Invalid auth details\n");
                                        free(clearauth);
                                        return -1;
                                }
                        }
                        else {
                                fprintf(stderr,"Invalid Authorization header");
                                return -1;
                        }
                }
                h = h->next;
        }
        return 0;
}

/* free request info */
void free_request()
{
        free_headers(request->headers);
        free(request->httpv);
        free(request->method);
        free(request->res);
        free(request->authuser);
        free(request->authpass);
}

/* free request headers */
void free_headers(http_header_t *h)
{
        http_header_t *tmp;

        while (h != NULL) {
                free(h->key);
                free(h->value);
                tmp = h;
                free(tmp);
                h = h->next;
        }
}
