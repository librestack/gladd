/*
 * utils.c - some handy functions that don't fit elsewhere
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

/* recursively create directory and parents */
int rmkdir(char *dir, mode_t mode)
{
        char *parent;
        int i;
        int r = 0;

        if (mkdir(dir, mode) != 0) {
                if (errno == ENOENT) {
                        parent = strdup(dir);
                        for (i=strlen(parent);i>0;--i) {
                                if (parent[i] == '/') {
                                        parent[i] = '\0';
                                        if ((r=rmkdir(parent,mode))!= 1) break;
                                }
                        }
                        free(parent);
                        if (r == 0) return 0;
                        rmkdir(dir, mode);
                }
                else if (errno != EEXIST) {
                        syslog(LOG_ERR, "Error creating directory '%s': %s",
                                dir, strerror(errno));
                        return 0;
                }
                else {
                        return -1;
                }
        }
        else {
                syslog(LOG_DEBUG, "Directory '%s' created", dir);
                return 1;
        }
        return -1;
}
