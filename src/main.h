/* 
 * main.h
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

#ifndef __GLADD_MAIN_H__
#define __GLADD_MAIN_H__ 1

#define BACKLOG 10  /* how many pending connectiong to hold in queue */
#define BUFSIZE 8096
#define LOCKFILE ".gladd.lock"
#define PROGRAM "gladd"
#define DEFAULT_CONFIG "/etc/gladd.conf"

int sockme;

int main (void);
void respond (int fd, char *response);

#endif /* __GLADD_MAIN_H__ */
