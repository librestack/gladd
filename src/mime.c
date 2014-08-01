/* 
 * mime.c
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
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>
#include "mime.h"

int read_mimefile(char *filename, char *mimetype)
{
        char *mimefile;
        int fd;
        struct stat stat_buf;

        /* check if filename.mime exists */
        asprintf(&mimefile, "%s.mime", filename);
        fd = open(mimefile, O_RDONLY);
        if (fd == -1) {
                free(mimefile);
                return 0;
        }
        else {
                if (fstat(fd, &stat_buf) == 0) {
                        if (S_ISREG(stat_buf.st_mode)) {
                                mimetype = malloc(stat_buf.st_size + 1);
                                if (read(fd, mimetype, stat_buf.st_size)
                                                != stat_buf.st_size)
                                {
                                        syslog(LOG_ERR, "failed to read %s",
                                                        mimefile);
                                        free(mimefile);
                                        free(mimetype);
                                        close(fd);
                                        return 0;
                                }
                                mimetype[stat_buf.st_size] = '\0';
                                syslog(LOG_DEBUG, "mime type set from file: %s",
                                        mimetype);
                        }
                }
                close(fd);
        }
        free(mimetype);
        free(mimefile);
        return 1;
}

char *get_mime_type(char *filename)
{
        char *fileext;
        char *mimetype = NULL;

        if (read_mimefile(filename, mimetype)) return mimetype;

        if (strrchr(filename, '.')) {
                fileext = strdup(strrchr(filename, '.')+1);
                if (strcmp(fileext, "html") == 0) {
                        asprintf(&mimetype, "text/html");
                }
                else if (strcmp(fileext, "gif") == 0) {
                        asprintf(&mimetype, "image/gif");
                }
                else if (strcmp(fileext, "png") == 0) {
                        asprintf(&mimetype, "image/png");
                }
                else if (strcmp(fileext, "xml") == 0) {
                        asprintf(&mimetype, "application/xml");
                }
                else if (strcmp(fileext, "xsl") == 0) {
                        asprintf(&mimetype, "application/xml");
                }
                else if (strcmp(fileext, "css") == 0) {
                        asprintf(&mimetype, "text/css");
                }
                else if (strcmp(fileext, "js") == 0) {
                        asprintf(&mimetype, "text/javascript");
                }
                else if (strcmp(fileext, "pdf") == 0) {
                        asprintf(&mimetype, "application/pdf");
                }
                else if (strcmp(fileext, "ogv") == 0) {
                        asprintf(&mimetype, "video/ogg");
                }
                else {
                        asprintf(&mimetype, "%s", MIME_DEFAULT);
                }
                free(fileext);
        }
        return mimetype;
}
