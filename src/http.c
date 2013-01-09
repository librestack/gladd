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
#include <stdio.h>
#include <stdlib.h>
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

