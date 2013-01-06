/* 
 * config.h
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

#ifndef __GLADD_CONFIG_H__
#define __GLADD_CONFIG_H__ 1

#include <stdio.h>

typedef struct url_t {
        char *type;
        char *url;
        char *path;
        struct url_t *next;
} url_t;

typedef struct config_t {
        long debug;
        long port;
        struct url_t *urls;
} config_t;

extern config_t *config;

FILE *open_config(char *configfile);
int process_config_line(char *line);
int read_config(char *configfile);
int set_config_defaults();
int set_config_long(long *confset, char *keyname, long i, long min, long max);
void free_urls();

#endif /* __GLADD_CONFIG_H__ */
