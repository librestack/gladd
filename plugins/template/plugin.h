/* 
 * Example plugin code
 *
 * plugin.h
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

#ifndef _GLADD_PLUGIN_H
#define _GLADD_PLUGIN_H 1
extern char buf[4096];

#define RESPONSE "<xml><response><code>%i</code><message>%s</message></response>\n"

enum httpcode {
        HTTP_OK                         = 200,
        HTTP_BAD_REQUEST                = 400,
        HTTP_UNAUTHORIZED               = 401,
        HTTP_FORBIDDEN                  = 403,
        HTTP_NOT_FOUND                  = 404,
        HTTP_METHOD_NOT_ALLOWED         = 405,
        HTTP_LENGTH_REQUIRED            = 411,
        HTTP_UNSUPPORTED_MEDIA_TYPE     = 415,
        HTTP_INTERNAL_SERVER_ERROR      = 500,
        HTTP_NOT_IMPLEMENTED            = 501,
        HTTP_VERSION_NOT_SUPPORTED      = 505
};

enum xmlstatus {
        XML_STATUS_OK,
        XML_STATUS_INVALID,
        XML_STATUS_UNKNOWN
};

void xml_cleanup();
char *xml_element(char *element);
void xml_free(void *ptr);
int xml_parse(char *xml);
size_t read_input(FILE *f);
ssize_t write_output(FILE *f, int result);

#endif /*_GLADD_PLUGIN_H */
