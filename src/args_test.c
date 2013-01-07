/* 
 * args_test.c - unit tests for args.c
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

#include "args.h"
#include "args_test.h"
#include "minunit.h"

/* ensure process_args() returns -1 when args invalid */
char *test_args_invalid()
{
        char *argv[] = {"invalid"};
        int argc = sizeof(argv) / sizeof(int);
        mu_assert("Ensure process_args() returns -1 when args invalid", 
                process_args(argc , argv) == -1);
        return 0;
}
