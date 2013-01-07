/*
 * signals.c - handle process signals
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

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/wait.h>
#include <unistd.h>
#include "config.h"
#include "signals.h"

/* clean up zombie children */
void sigchld_handler (int signo)
{
        int status;
        wait(&status);
}

/* catch SIGTERM and clean up */
void sigterm_handler (int signo)
{
        close(sockme);
        syslog(LOG_INFO, "Received SIGTERM.  Shutting down.");
        closelog();
        free_urls();
        exit(EXIT_SUCCESS);
}

/* catch HUP and reload config */
void sighup_handler (int signo)
{
        syslog(LOG_INFO, "Received SIGHUP.  Reloading config.");

        if (read_config(DEFAULT_CONFIG) != 0) {
                syslog(LOG_ERR, "Config reload failed.");
        }

}
