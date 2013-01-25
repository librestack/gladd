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
#include "db.h"
#include <string.h>
#include <syslog.h>

/* 
 * check if authorized for requested method and url
 * default is to return 403 Forbidden
 * a return value of 0 is allow.
 */
int check_auth(http_request_t *r)
{
        acl_t *a;

        a = config->acls;
        while (a != NULL) {
                syslog(LOG_DEBUG, 
                        "Checking acls for %s %s ... trying %s %s", 
                        r->method, r->res, a->method, a->url);
                /* ensure method and url matches */
                if ((strncmp(r->res, a->url, strlen(a->url)) == 0) &&
                    (strncmp(r->method, a->method, strlen(r->method)) == 0))
                {
                        if (strncmp(a->auth, "*", strlen(a->auth)) == 0) {
                                syslog(LOG_DEBUG, "acl matches");
                                if (strcmp(a->type, "require") == 0) {
                                        /* auth require - needs more checks */
                                        return check_auth_require("ldap",
                                                r->authuser, 
                                                r->authpass);
                                }
                                /* acl matches, return 0 if allow, else 403 */
                                return 
                                    strncmp(a->type, "allow", 5) == 0 ? 0:403;
                        }
                }
                a = a->next;
        }
        if (a != NULL) {
                return 0;
        }

        syslog(LOG_DEBUG, "no acl matched");
        return 403; /* default is to deny access */
}

/* verify auth requirements met */
int check_auth_require(char *alias, char *authuser, char *authpass)
{
        auth_t *a;

        if (! (a = getauth(alias))) {
                syslog(LOG_ERR, 
                        "Invalid alias '%s' supplied to check_auth_require()",
                        alias);
                return 500;
        }

        if (strcmp(a->type, "ldap") == 0) {
                return db_test_bind(getdb(a->db), getsql(a->sql), a->bind,
                        authuser, authpass);
        }
        else {
                syslog(LOG_ERR,
                        "Invalid auth type '%s' in check_auth_require()",
                        a->type);
                return 500;
        }

        return 0;
}
