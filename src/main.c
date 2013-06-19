/*
 * main.c - entry point for gladd httpd
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

#include <errno.h>
#include <grp.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <syslog.h>
#include <unistd.h>

#include "args.h"
#include "config.h"
#include "handler.h"
#include "main.h"
#include "server.h"
#include "signals.h"

int main (int argc, char **argv)
{
        int getaddrinfo(const char *node,
                        const char *service,
                        const struct addrinfo *hints,
                        struct addrinfo **res);

        int havelock = 0;
        int lockfd;
        char *errmsg;

        /* set up signal handlers */
        if (sighandlers() == -1) {
                fprintf(stderr, "Failed to set up signals. Exiting.\n");
                exit(EXIT_FAILURE);
        }

        /* check commandline args */
        if (process_args(argc, argv) == -1)
                exit(EXIT_FAILURE);

        /* obtain lockfile */
        if ((havelock = obtain_lockfile(&lockfd)) != 0)
                exit(havelock);

        /* open syslogger */
        openlog(PROGRAM, LOG_CONS|LOG_PID, LOG_DAEMON);

        syslog(LOG_INFO, "Starting up.");

        /* read config */
        if (read_config(DEFAULT_CONFIG) != 0) {
                asprintf(&errmsg,"Failed to read config on startup. Exiting.");
                syslog(LOG_ERR, "%s", errmsg);
                fprintf(stderr, "%s\n", errmsg);
                free(errmsg);
                exit(EXIT_FAILURE);
        }

        return server_start(lockfd);
}

/* return name of lockfile - free() after use */
char *getlockfilename()
{
        char *lockfile;

        if (geteuid() == 0) {
                /* we are root, put lockfile in /var/run */
                asprintf(&lockfile, "%s", LOCKFILE_ROOT);
        }
        else {
                /* not root, put pidfile in user home */
                asprintf(&lockfile, "%s/%s", getenv("HOME"), LOCKFILE_USER);
        }
        return lockfile;
}

/* obtain lockfile */
int obtain_lockfile(int *lockfd)
{
        int retval = 0;
        char *lockfile;

        lockfile = getlockfilename();

        *lockfd = open(lockfile, O_RDWR | O_CREAT, 
                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH );
        if (*lockfd == -1) {
                printf("Failed to open lockfile %s\n", lockfile);
                retval = EXIT_FAILURE;
        }
        if (flock(*lockfd, LOCK_EX|LOCK_NB) != 0) {
                if (g_signal != 0) {
                        /* signal (SIGHUP, SIGTERM etc.) requested */
                        retval = signal_gladd(*lockfd);
                        if (g_signal == SIGUSR1) {
                                signal_wait(); /* wait for a response */
                        }
                        exit(retval);
                }
                printf("%s already running\n", PROGRAM);
                retval = EXIT_FAILURE;
        }
        else if (g_signal != 0) {
                /* wanted to send a signal, but daemon not running */
                printf("%s not running\n", PROGRAM);
                /* exit with success if we were asking status, else fail */
                retval = g_signal == SIGUSR1 ? EXIT_SUCCESS : EXIT_FAILURE;
                exit(retval);
        }
        free(lockfile);
        return retval;
}
