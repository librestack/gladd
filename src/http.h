/* 
 * http.h
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

#ifndef __GLADD_HTTP_H__
#define __GLADD_HTTP_H__ 1

#define HTTPKEYS (sizeof httpcode / sizeof (struct http_status))
#define HTTP_RESPONSE "HTTP/1.1 %1$i %2$s\nServer: gladd\nConnection: close\nContent-Type: %3$s%4$s\n\n<html><body><h1>%1$i %2$s</h1>\n</body>\n</html>\n"
#define MAX_RESOURCE_LEN 256

#include <sys/types.h>
#include "config.h"

typedef enum {
        HTTP_BAD_REQUEST                = 400,
        HTTP_NOT_FOUND                  = 404,
        HTTP_LENGTH_REQUIRED            = 411,
        HTTP_INTERNAL_SERVER_ERROR      = 500
} http_status_code_t;

struct http_status {
        int code;
        char *status;
};

typedef struct http_request_t {
        char *httpv;            /* HTTP version                          */
        char *method;           /* HTTP request method (GET, POST etc.)  */
        char *res;              /* resource (url) requested              */
        ssize_t bytes;          /* bytes recv()'d                        */
        char *authuser;         /* username supplied for http basic auth */
        char *authpass;         /* password    "      "   "     "    "   */
        keyval_t *headers;      /* client request headers                */
        keyval_t *data;         /* client request data                   */
} http_request_t;

extern http_request_t *request;

void bodyline(http_request_t *r, char *line);
int check_content_length(http_request_t *r, http_status_code_t *err);
void free_request(http_request_t *r);
char *decode64(char *str);
struct http_status get_status(int code);
void http_add_request_data(http_request_t *r, char *key, char *value);
http_request_t *http_init_request();
url_t  *http_match_url(http_request_t *r);
keyval_t *http_set_keyval (char *key, char *value);
void http_response(int sock, int code);
char *http_get_header(http_request_t *r, char *key);
http_request_t *http_read_request(char *buf, ssize_t bytes, int *hcount,
        http_status_code_t *err);
void http_set_request_method(http_request_t *r, char *method);
void http_set_request_resource(http_request_t *r, char *res);
int http_validate_headers(http_request_t *r, http_status_code_t *err);

#endif /* __GLADD_HTTP_H__ */
