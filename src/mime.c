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

char *get_mime_type(char *filename)
{
        char *fileext;
        char *mimefile;
        char *mimetype;
        int fd;
        struct stat stat_buf;

        /* first, check if filename.mime exists */
        asprintf(&mimefile, "%s.mime", filename);
        fd = open(mimefile, O_RDONLY);
        if (fd != -1) {
                fstat(fd, &stat_buf);
                if (S_ISREG(stat_buf.st_mode)) {
                        mimetype = malloc(stat_buf.st_size + 1);
                        read(fd, mimetype, stat_buf.st_size);
                        close(fd);
                        free(mimefile);
                        syslog(LOG_DEBUG, "mime type set from file: %s", 
                                mimetype);
                        mimetype[stat_buf.st_size] = '\0';
                        return mimetype;
                }
        }
        free(mimefile);

        /* TODO: use apache's mime.types file */
        if (strrchr(filename, '.')) {
                fileext = strdup(strrchr(filename, '.')+1);
                if (strncmp(fileext, "html", 4) == 0) {
                        asprintf(&mimetype, "text/html");
                }
                else if (strncmp(fileext, "gif", 3) == 0) {
                        asprintf(&mimetype, "image/gif");
                }
                else if (strncmp(fileext, "png", 3) == 0) {
                        asprintf(&mimetype, "image/png");
                }
                else if (strncmp(fileext, "xml", 3) == 0) {
                        asprintf(&mimetype, "application/xml");
                }
                else if (strncmp(fileext, "xsl", 3) == 0) {
                        asprintf(&mimetype, "application/xml");
                }
                else if (strncmp(fileext, "css", 3) == 0) {
                        asprintf(&mimetype, "text/css");
                }
                else if (strncmp(fileext, "js", 2) == 0) {
                        asprintf(&mimetype, "text/javascript");
                }
                else if (strncmp(fileext, "pdf", 3) == 0) {
                        asprintf(&mimetype, "application/pdf");
                }
                else if (strncmp(fileext, "ogv", 3) == 0) {
                        asprintf(&mimetype, "video/ogg");
                }
                else {
                        asprintf(&mimetype, "%s", MIME_DEFAULT);
                }
                free(fileext);
        }
        return mimetype;
}
