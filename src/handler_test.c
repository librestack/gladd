/* 
 * handler_test.c - unit tests for handler.c
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
#include "config.h"
#include "handler_test.h"
#include "http.h"
#include "minunit.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


char *test_handler_plugin()
{
        char pbuf[BUFSIZE] = "";
        int sv[2];
        struct timeval tv;
        url_t *u;

        /* create a pair of connected sockets */
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1) {
                perror("socketpair");
        }

        /* set up the request etc. */
        request = http_init_request();
        asprintf(&request->res, "/plugin1/1234");

        u = malloc(sizeof(struct url_t));
        asprintf(&u->type, "plugin");
        asprintf(&u->method, "GET");
        asprintf(&u->url, "/plugin1/*");
        asprintf(&u->path, "echo $1 | md5sum");

        mu_assert("Plugin (GET) - md5sum", response_plugin(sv[1], u) == 0);

        /* set socket timeout */
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        setsockopt(sv[0], SOL_SOCKET, SO_RCVTIMEO,
                (char *)&tv, sizeof(struct timeval));

        read(sv[0], pbuf, BUFSIZE);

        /* test the response */
        mu_assert("... correct result",
                strcmp(pbuf, "e7df7cd2ca07f4f1ab415d457a6e1c13  -\n") == 0);
       
        /* tidy up */
        free(u->type);
        free(u->method);
        free(u->url);
        free(u->path);
        free(u);
        free_request(request);

        return 0;
}
