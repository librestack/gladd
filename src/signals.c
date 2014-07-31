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
#include "main.h"
#include "server.h"
#include "signals.h"

int handler_procs = 0;

/* make sure we don't leave zombies - called by atexit() and sigterm */
void killhandlerprocs()
{
        for (;handler_procs > 0; handler_procs--) {
                kill(0, SIGKILL);
        }
}

/* set up signal handling */
int sighandlers()
{
        int ret;
        struct sigaction act1, act2, oldact;

        signal(SIGCHLD, sigchld_handler);
        signal(SIGINT, sigint_handler);
        signal(SIGTERM, sigterm_handler);
        signal(SIGHUP, sighup_handler);

        act1.sa_flags = SA_SIGINFO;
        act1.sa_sigaction = sigusr1_handler;
        ret = sigaction(SIGUSR1, &act1, &oldact);

        act2.sa_flags = SA_SIGINFO;
        act2.sa_sigaction = sigusr2_handler;
        ret = sigaction(SIGUSR2, &act2, &oldact);

        return ret;
}

/* clean up zombie children */
void sigchld_handler (int signo)
{
        int status;
        while (waitpid(-1, &status, WNOHANG) > 0);
        handler_procs--;
}

/* catch SIGINT and clean up */
void sigint_handler (int signo)
{
        sigterm_handler(signo);
}

/* catch SIGTERM and clean up */
void sigterm_handler (int signo)
{
        close(sockme);
        syslog(LOG_INFO, "Received SIGTERM.  Shutting down.");
        killhandlerprocs();
        closelog();
        free_config();
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

/* catch SIGUSR1 - request for status */
void sigusr1_handler (int signo, siginfo_t *si, void *ucontext)
{
        int ret;
        union sigval value;

        syslog(LOG_INFO, "Client status request [%i]", si->si_pid);

        /* respond to client */
        value.sival_int = get_hits();
        ret = sigqueue(si->si_pid, SIGUSR2, value);
        if (ret == -1)
                syslog(LOG_ERR, "ERROR: failed to send SIGUSR2");
}

/* catch SIGUSR2 - response to status enquiry */
void sigusr2_handler (int signo, siginfo_t *si, void *ucontext)
{
        printf("%s running, %i hits\n", PROGRAM, si->si_int);
        exit(EXIT_SUCCESS);
}

/* send signal to running gladd process */
int signal_gladd (int lockfd)
{
        char buf[sizeof(long)];
        int pid;

        if (read(lockfd, &buf, sizeof(buf)) == -1) {
                fprintf(stderr, "Failed to read pid\n");
                return EXIT_FAILURE;
        }
        if (sscanf(buf, "%i", &pid) == 1) {
                return kill(pid, g_signal);
        }
        else {
                fprintf(stderr, "Invalid pid\n");
                return EXIT_FAILURE;
        }
}

/* catch response from server */
void signal_wait() {
        sleep(3); /* give up after 3s */
        printf("Timeout.  No response from server\n");
        exit(EXIT_FAILURE);
}
