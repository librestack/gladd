/* 
 * auth.c - authentication functions
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

#include "auth.h"
#include "config.h"
#include <string.h>
#include <syslog.h>

/* 
 * check if authorized for requested method and url
 * default is to return 403 Forbidden
 * a return value of 0 is allow.
 */
int check_auth(char *method, char *url)
{
        acl_t *a;

        a = config->acls;
        while (a != NULL) {
                syslog(LOG_DEBUG, 
                        "Checking acls for %s %s ... trying %s %s", 
                        method, url, a->method, a->url);
                if ((strncmp(url, a->url, strlen(a->url)) == 0) &&
                    (strncmp(method, a->method, strlen(method)) == 0))
                {
                        if (strncmp(a->auth, "*", strlen(a->auth)) == 0) {
                                syslog(LOG_DEBUG, "acl matches");
                                /* acl matches, return 0 if allow, else -1 */
                                return 
                                    strncmp(a->type, "allow", 5) == 0 ? 0:403;
                        }
                }
                a = a->next;
        }
        if (a != NULL) {
                return 0;
        }

        return 403; /* default is to deny access */
}
