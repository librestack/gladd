/* 
 * auth_test.c - unit tests for auth.c
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
#include "auth.h"
#include "auth_test.h"
#include "minunit.h"
#include <stdlib.h>

/* ensure check_auth() returns 403 by default */
char *test_auth_default()
{
        http_request_t *r;

        r = http_init_request();

        http_set_request_method(r, "POST");
        http_set_request_resource(r, "/blah/");
        mu_assert("ensure check_auth() returns 403 Forbidden by default", 
                check_auth(r) == 403);

        free_request(r);

        return 0;
}

char *test_auth_deny()
{
        http_request_t *r;

        r = http_init_request();

        http_set_request_method(r, "GET");
        http_set_request_resource(r, "/static/secret.html");
        mu_assert("ensure GET /static/secret.html returns 401 Unauthorized", 
                check_auth(r) == 401);

        http_set_request_resource(r, "/denyme.html");
        mu_assert("ensure GET /denyme.html returns 403 Forbidden", 
                check_auth(r) == 403);

        http_set_request_method(r, "POST");
        http_set_request_resource(r, "/static/index.html");
        mu_assert("ensure POST /static/index.html returns 403 Forbidden", 
                check_auth(r) == 403);

        http_set_request_method(r, "DELETE");
        http_set_request_resource(r, "/sqlview/");
        mu_assert("ensure DELETE /sqlview/ returns 403 Forbidden", 
                check_auth(r) == 403);

        free_request(r);

        return 0;
}

char *test_auth_allow()
{
        http_request_t *r;

        r = http_init_request();

        http_set_request_method(r, "GET");
        http_set_request_resource(r, "/static/index.html");
        mu_assert("ensure GET /static/index.html allowed", 
                check_auth(r) == 0);

        http_set_request_resource(r, "/sqlview/");
        mu_assert("ensure GET /sqlview/ allowed", 
                check_auth(r) == 0);

        http_set_request_method(r, "POST");
        mu_assert("ensure POST /sqlview/ allowed", 
                check_auth(r) == 0);

        free_request(r);

        return 0;
}

char *test_auth_require()
{
        http_request_t *r;

        r = http_init_request();

        mu_assert("check_auth_require() - fail on invalid alias",
                check_auth_require("invalid", r) != 0);

        asprintf(&r->authuser, "betty");
        asprintf(&r->authpass, "false");
        mu_assert("check_auth_require() - fail with invalid credentials",
                check_auth_require("ldap", r) != 0);
        free_request(r);

        r = http_init_request();
        asprintf(&r->authuser, "betty");
        asprintf(&r->authpass, "ie5a8P40");
        mu_assert("check_auth_require() - successful ldap bind",
                check_auth_require("ldap", r) == 0);
        free_request(r);

        r = http_init_request();
        asprintf(&r->authuser, "bravo");
        asprintf(&r->authpass, "wrongpassword");
        mu_assert("check_auth_require() - invalid user/pass combo",
                check_auth_require("user", r) != 0);
        free_request(r);

        r = http_init_request();
        asprintf(&r->authuser, "bravo");
        asprintf(&r->authpass, "bravosecret");
        mu_assert("check_auth_require() - successful user/pass combo",
                check_auth_require("user", r) == 0);
        free_request(r);

        return 0;
}

char *test_auth_patterns()
{
        http_request_t *r;

        read_config("test_auth_patterns_00.conf");
        r = http_init_request();
        asprintf(&r->method, "GET");
        asprintf(&r->res, "/static/secret.html");
        asprintf(&r->authuser, "bravo");
        asprintf(&r->authpass, "bravosecret");
        mu_assert("check_auth() - valid user will fail when ldap required",
                check_auth(r) == HTTP_UNAUTHORIZED);
        free_request(r);
        free_config();

        read_config("test_auth_patterns_01.conf");
        r = http_init_request();
        asprintf(&r->method, "GET");
        asprintf(&r->res, "/static/secret.html");
        asprintf(&r->authuser, "bravo");
        asprintf(&r->authpass, "bravosecret");
        mu_assert("check_auth() - valid user sufficient",
                check_auth(r) == 0);
        free_request(r);
        free_config();

        return 0;
}

char *test_auth_groups()
{
        http_request_t *r;
        read_config("test_auth_groups_00.conf");
        r = http_init_request();

        mu_assert("Check user in group (invalid group returns -1)",
                ingroup("alpha", "invalid") == -1);

        mu_assert("User alpha in group1", ingroup("alpha", "group1"));
        mu_assert("User bravo not in group1", !ingroup("bravo", "group1"));
        mu_assert("User charlie not in group1", !ingroup("charlie", "group1"));
        mu_assert("Invalid user delta not in group1",
                !ingroup("delta", "group1"));

        mu_assert("User alpha in group2", ingroup("alpha", "group2"));
        mu_assert("User bravo in group2", ingroup("bravo", "group2"));
        mu_assert("User charlie not in group2", !ingroup("charlie", "group2"));

        mu_assert("User alpha not in group3", !ingroup("alpha", "group3"));
        mu_assert("User bravo not in group3", !ingroup("bravo", "group3"));
        mu_assert("User charlie in group3", ingroup("charlie", "group3"));

        free_request(r);
        free_config();

        return 0;
}
