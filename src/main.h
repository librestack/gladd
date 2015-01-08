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

#define BACKLOG 100  /* how many pending connectiong to hold in queue */
#define LOCKFILE_USER ".gladd.pid"
#define LOCKFILE_ROOT "/var/run/gladd.pid"
#define PROGRAM "gladd"
#define VERSION "0.2.4"

#define _unused(x) ((void)x)

int sockme;

int main (int argc, char **argv);
char *getlockfilename();
int obtain_lockfile(int *lockfd);

#endif /* __GLADD_MAIN_H__ */
