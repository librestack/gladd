/*
 * string.c - handy string functions
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

#include "string.h"
#include <ctype.h>
#include <string.h>

/* trim leading spaces from string */
char *lstrip(char *str)
{
        /* trim leading space */
        while (isspace(*str)) str++;

        return str;
}

/* trim trailing spaces from string */
char *rstrip(char *str)
{
        #include <stdio.h>
        char *end;

        end = strlen(str) + str - 1;
        while (end >= str && isspace(*end)) end--;

        /* poke in null terminator */
        *(end + 1) = '\0';

        return str;
}

/* trim leading and trailing spaces from string */
char *strip(char *str)
{
        return lstrip(rstrip(str));
}
