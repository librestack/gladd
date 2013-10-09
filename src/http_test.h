/* 
 * http_test.h - unit tests for http.c
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

#ifndef __GLADD_HTTP_TEST__H__
#define __GLADD_HTTP_TEST__H__ 1

#include "http.h"

char *test_http_read_request_get();
char *test_http_read_request_post();
char *test_http_read_request_data();
char *test_http_readline();
char *test_http_postdata_invalid();
char *test_http_postdata_checks();
char *test_http_read_request_post_xml();
char *test_http_read_request_post_large();

#endif /* __GLADD_HTTP_TEST__H__ */
