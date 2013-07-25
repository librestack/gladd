/* 
 * auth_test.h - unit tests for auth.c
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

#ifndef __GLADD_AUTH_TEST_H__
#define __GLADD_AUTH_TEST_H__ 1

char *test_auth_default();
char *test_auth_deny();
char *test_auth_allow();
char *test_auth_require();
char *test_auth_patterns();
char *test_auth_groups_00();
char *test_auth_groups_01();
char *test_auth_groups_02();

#endif /* __GLADD_AUTH_TEST_H__ */
