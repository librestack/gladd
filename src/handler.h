/* 
 * handler.h
 *
 * this file is part of GLADD
 *
 * Copyright (c) 2012-2019 Brett Sheffield <brett@gladserv.com>
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

#ifndef __GLADD_HANDLER_H__
#define __GLADD_HANDLER_H__ 1

#define RESPONSE_200 "HTTP/1.1 200 OK\nServer: %s\nContent-Type: %s\n\n%s"

#include <sys/socket.h>
#include "http.h"

typedef enum {
        HANDLER_OK                      = 0,
        HANDLER_CLOSE_CONNECTION        = 1,
        HANDLER_UPGRADE_INVALID_METHOD  = 2,
        HANDLER_UPGRADE_INVALID_HTTP_VERSION = 3,
        HANDLER_UPGRADE_NO_HOST_HEADER  = 4,
        HANDLER_UPGRADE_INVALID_UPGRADE = 5,
        HANDLER_UPGRADE_INVALID_CONN    = 6,
        HANDLER_UPGRADE_MISSING_KEY     = 7,
        HANDLER_UPGRADE_INVALID_WEBSOCKET_VERSION = 8,
        HANDLER_INVALID_WEBSOCKET_PROTOCOL = 9
} handler_result_t;

void *get_in_addr(struct sockaddr *sa);
db_t   *getdbv(char *alias);
void handle_connection(int sock, struct sockaddr_storage their_addr);
handler_result_t handle_request(int sock, char *s);
int handler_upgrade_connection_check(http_request_t *r);
size_t rcv(int sock, void *data, size_t len, int flags);
ssize_t snd(int sock, void *data, size_t len, int flags);
ssize_t snd_blank_line(int sock);
ssize_t snd_string(int sock, char *str, ...);
void respond (int fd, char *response);
int send_file(int sock, char *path, http_status_code_t *err);
void setcork(int sock, int state);
void set_headers(char **r);
#ifdef _GIT
http_status_code_t response_git(int sock, url_t *u);
#endif
#ifndef _NGLADDB
http_status_code_t response_keyval(int sock, url_t *u);
#ifndef _NLDIF
http_status_code_t response_ldif(int sock, url_t *u);
#endif /* _NLDIF */
http_status_code_t response_sqlview(int sock, url_t *u);
http_status_code_t response_sqlexec(int sock, url_t *u);
#endif /* _NGLADDB */
http_status_code_t response_plugin(int sock, url_t *u);
http_status_code_t response_static(int sock, url_t *u);
http_status_code_t response_upload(int sock, url_t *u);
http_status_code_t response_upgrade(int sock, url_t *u);
http_status_code_t response_xslt(int sock, url_t *u);

#endif /* __GLADD_HANDLER_H__ */
