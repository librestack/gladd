/* 
 * gnutls.c
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

#ifndef __GLADD_GNUTLS_H__
#define __GLADD_GNUTLS_H__ 1

#include <gnutls/gnutls.h>

#define GNUTLS_DEBUG_LEVEL 10 /* 0 = off, 10+ = all debug enabled */

gnutls_dh_params_t dh_params;
gnutls_session_t session;

int generate_dh_params(void);
void do_tls_handshake(int fd);
void setcork_ssl(int state);
int sendfile_ssl(int sock, int fd, size_t size);
size_t ssl_send(char *msg, size_t len);
size_t ssl_recv(char *b, int bytes);
void ssl_setup();
void ssl_cleanup();
#endif /* __GLADD_GNUTLS_H__ */
