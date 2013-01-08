/* 
 * config_test.c - unit tests for config.c
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

#include "config_test.h"
#include "config.h"
#include "minunit.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/* process_config_line() must return 1 if line is a comment */
char *test_config_skip_comment()
{
        mu_assert("Ensure comments are skipped by config parser",
                process_config_line("# This line is a comment\n") == 1);
        return 0;
}

/* process_config_line() must return 1 if line is blank */
char *test_config_skip_blank()
{
        mu_assert("Ensure blank lines are skipped by config parser",
                process_config_line(" \t \n") == 1);
        return 0;
}

/* process_config_line() must return -1 if line is invalid */
char *test_config_invalid_line()
{
        mu_assert("Ensure invalid lines return error",
                process_config_line("gibberish") == -1);
        return 0;
}

/* test opening config file */
char *test_config_open_success()
{
        FILE *fd;

        fd = open_config("test.conf");
        mu_assert("Open test.conf for reading", fd != NULL);
        fclose(fd);
        return 0;
}

/* ensure failing to open config returns an error */
char *test_config_open_fail()
{
        mu_assert("Ensure failure to open file returns error", 
                read_config("fake.conf") == 1);
        return 0;
}

/* test default value of debug = 0 */
char *test_config_default_debug_value()
{
        set_config_defaults();
        mu_assert("Ensure default debug=0", config->debug == 0);
        return 0;
}

/* test default value of port=8080 */
char *test_config_default_port_value()
{
        mu_assert("Ensure default port=8080", config->port == 8080);
        return 0;
}

/* ensure value of debug is set from config */
char *test_config_set_debug_value()
{
        read_config("test.conf");
        mu_assert("Ensure debug is set from config", config->debug == 1);
        return 0;
}

/* ensure value of port is set from config */
char *test_config_set_port_value()
{
        mu_assert("Ensure port is set from config", config->port == 3000);
        return 0;
}

/* read url directive from config file */
char *test_config_read_url_static()
{
        mu_assert("Ensure urls are read from config", 
                        strncmp(config->urls->type, "static", 6) == 0);
        return 0;
}

/* test successive reads of url->next */
char *test_config_read_url_static_next()
{
        url_t *u;
        u = config->urls;

        fprintf(stderr, "Config: %s\n", config->urls->url);
        fprintf(stderr, "First: %s\n", u->url);
        mu_assert("Reading first url from config", 
                        strncmp(u->url, "/static/", strlen(u->url)) == 0);

        u = u->next;
        fprintf(stderr, "Second: %s\n", u->url);
        mu_assert("Reading second url from config", 
                        strncmp(u->url, "/static2/", strlen(u->url)) == 0);
        u = u->next;
        fprintf(stderr, "Third: %s\n", u->url);
        mu_assert("Reading third url from config", 
                        strncmp(u->url, "/static3/", strlen(u->url)) == 0);

        mu_assert("Ensure final url->next returns NULL", u->next == NULL);

        free_urls(); /* call this only after all url tests are complete */
        
        return 0;
}

/* ensure add_acl() rejects junk */
char *test_config_add_acl_invalid()
{
        mu_assert("ensure add_acl() rejects junk", add_acl("invalid junk"));
        return 0;
}

/* test acl can be read from config */
char *test_config_acl_allow_all()
{
        /* read and check first acl */
        mu_assert("test acl->type is read from config", 
                    strncmp(config->acls->type, "allow", 
                                strlen(config->acls->type)) == 0);
        mu_assert("test acl->url is read from config", 
                    strncmp(config->acls->url, "/", 
                                strlen(config->acls->url)) == 0);
        return 0;
}
