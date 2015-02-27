/* 
 * handler_test.c - unit tests for handler.c
 *
 * this file is part of GLADD
 *
 * Copyright (c) 2012-2015 Brett Sheffield <brett@gladserv.com>
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
#include "string.h"
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
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

#ifndef _NAUTH
        /* test the response */
        mu_assert("... correct result", strcmp(pbuf, "HTTP/1.1 200 OK\r\nServer: gladd\r\nConnection: close\r\nTransfer-Encoding: chunked\r\n\r\n24\r\ne7df7cd2ca07f4f1ab415d457a6e1c13  -\n\r\n0\r\n\r\n") == 0);
#endif

        /* tidy up */
        free(u->type);
        free(u->method);
        free(u->url);
        free(u->path);
        free(u);
        free_request(&request);

        return 0;
}

/* Test the handling of uploaded files in multipart MIME format */
char *test_handler_upload()
{
        uploadtest_t t;
        asprintf(&t.file, "../testdata/upload.multipart");
        asprintf(&t.sha1sum, "89665874c70a9e2bcd8a051ea21eca91342d50ba");
        asprintf(&t.msg, "Upload Test 1");
        asprintf(&t.response, " ");
        t.responsecode = 201;

        upload_test(t);

        free(t.file);
        free(t.sha1sum);
        free(t.msg);
        free(t.response);

        mu_assert("stop here", 1==0);
        return 0;
}

char *upload_test(uploadtest_t t)
{
        int sv[2];
        off_t offset = 0;
        struct stat stat_buf;
        int f;
        char pbuf[BUFSIZE] = "";
        pid_t pid;
        struct timeval tv;

        /* create a pair of connected sockets */
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1) {
                perror("socketpair");
        }

        pid = fork();
        if (pid == 0) { /* child */
                /* open the test file and write to socket */
                fprintf(stderr, "Opening file '%s'\n", t.file);
                f = open(t.file, O_RDONLY);
                if (f == -1) {
                        perror("open");
                        _exit(EXIT_FAILURE);
                }

                fstat(f, &stat_buf);
                fprintf(stderr, "Sending stream of size %i\n",
                        (int)stat_buf.st_size);
                sendfile(sv[0], f, &offset, stat_buf.st_size);
                close(f);
                fprintf(stderr, "Calling response_upload()\n");

                _exit(EXIT_SUCCESS); /* child exits */
        }
        mu_assert("Forking to run socket tests", pid != -1);

        mu_assert("Reading upload config",
                read_config("test.upload.conf") == 0);
        handle_request(sv[1], "127.0.0.1");

        /* set socket timeout */
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        setsockopt(sv[0], SOL_SOCKET, SO_RCVTIMEO,
                (char *)&tv, sizeof(struct timeval));

        /* read response */
        read(sv[0], pbuf, BUFSIZE);

        int code;
        sscanf(pbuf, "HTTP/1.1 %i", &code);

        fprintf(stderr, "HTTP response code %i\n", code);
        mu_assert("Check response code", code == t.responsecode);

        fprintf(stderr, "%s\n", pbuf);


        mu_assert("Verify sha1sum", 
                memsearch(pbuf, t.sha1sum, BUFSIZE) != NULL);

        free_request(&request);
        free_config();

        return 0;
}
