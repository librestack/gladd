/* 
 * tls.h
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

#ifndef __GLADD_TLS_H__
#define __GLADD_TLS_H__ 1

#define TLS_DEBUG_LEVEL 10 /* 0 = off, 10+ = all debug enabled */

#include <sys/types.h>

void do_tls_handshake(int fd);
int generate_dh_params(void);
int sendfile_ssl(int sock, int fd, size_t size);
void setcork_ssl(int state);
void ssl_cleanup(int fd);
size_t ssl_recv(char *b, int len);
size_t ssl_send(char *msg, size_t len);
void ssl_setup();

#endif /* __GLADD_TLS_H__ */
