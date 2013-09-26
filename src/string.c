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

#define _GNU_SOURCE
#include "string.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

/* trim leading spaces from string */
char *lstrip(char *str)
{
        /* trim leading space */
        while (isspace(*str)) str++;

        return str;
}

/* binary search.  Returns pointer to first occurance of string in binary data,
 * or NULL if not found */
char *memsearch(char *hay, char *pin, size_t size)
{
        char *ptr;

        for (ptr = hay; ptr != NULL; ptr++) {
                ptr = memchr(ptr, pin[0], size-(ptr-hay));
                if (ptr == NULL) break;                    /* not found */
                if (ptr + strlen(pin) > hay + size)
                        return NULL; /* end reached */
                if (memcmp(ptr, pin, strlen(pin)) == 0) {
                        break;
                }
        }

        return ptr;
}

/* search and replace 
 * NB: only replaces one instance of string
 * remember to free() the returned string
 */
char *replace(char *str, char *find, char *repl)
{
        char *p;
        char *newstr;
        char *tmp;

        if (!(p = strstr(str, find)))
                return strdup(str);

        tmp = strndup(str, p-str);
        asprintf(&newstr, "%s%s%s", tmp, repl, p+strlen(find));
        free(tmp);

        return newstr;
}

/* replace all instances of find in str with repl
 * remember to free() the returned string
 */
char *replaceall(char *str, char *find, char *repl)
{
        char *newstr;
        char *tmp;

        tmp = strdup(str);
        newstr = replace(tmp, find, repl);

        while (strcmp(newstr, tmp) != 0) {
                free(tmp);
                tmp = strdup(newstr);
                free(newstr);
                newstr = replace(tmp, find, repl);
        }
        free(tmp);

        return newstr;
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

/* tokenize string into array */
char **tokenize(int *segments, char **stringp, char *delim)
{
        int lenstring;
        char *i;
        char **tokens;
        int segs = 1;
        const int max_segs = 42;

        tokens = calloc(max_segs, sizeof(char *));
        tokens[0] = *stringp; /* we always return at least one segment */

        lenstring = strlen(*stringp);
        for (i=*stringp;i < *stringp + lenstring; i++) {
                if (strncmp(i, delim, strlen(delim)) == 0) {
                        i[0] = '\0'; /* replace delimiter with null */
                        i = i + strlen(delim) - 1;
                        tokens[segs] = i + 1;
                        segs++;
                        if (segs > max_segs)
                                break;
                }
        }

        *segments = segs;
        return tokens;
}
