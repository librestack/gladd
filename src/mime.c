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

#include <stdlib.h>
#include <string.h>
#include "mime.h"

void get_mime_type(char *mimetype, char *filename)
{
        char *fileext;

        /* default for unknown mime type */
        strcpy(mimetype, MIME_DEFAULT);

        /* TODO: use apache's mime.types file */
        if (strrchr(filename, '.')) {
                fileext = strdup(strrchr(filename, '.')+1);
                if (strncmp(fileext, "html", 4) == 0) {
                        strcpy(mimetype, "text/html");
                }
                else if (strncmp(fileext, "xsl", 3) == 0) {
                        strcpy(mimetype, "application/xml");
                }
                else if (strncmp(fileext, "css", 3) == 0) {
                        strcpy(mimetype, "text/css");
                }
                free(fileext);
        }
}
