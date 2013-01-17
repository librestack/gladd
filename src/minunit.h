/* 
 * minunit.h - simple unit testing macros
 * based on code from http://www.jera.com/techinfo/jtns/jtn002.html
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

#ifndef __MINUNIT_H__
#define __MINUNIT_H__ 1

#include <stdio.h>

#define mu_assert(message, test) do { \
        printf("%03i: %-65s ... ", tests_run, message); \
        if ((test)) { \
                printf("OK\n"); \
        } \
        else { \
                printf("FAIL\n"); \
                return message; \
        } \
        tests_run++; \
} while (0)

#define mu_run_test(test, ...) do { \
        char *message = test(__VA_ARGS__); \
        if (message) return message; \
} while (0)

extern int tests_run;

#endif /* __MINUNIT_H__ */
