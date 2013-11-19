/*
 * server.c - entry point for gladd httpd
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
#include "tls.h"
#include "server.h"
#include "signals.h"

int hits = 0;

int get_socket(struct addrinfo **servinfo)
{
        struct addrinfo hints;
        int sock;
        int status;
        int yes=1;
        char tcpport[5];

        /* set up hints */
        memset(&hints, 0, sizeof hints);           /* zero memory */
        hints.ai_family = AF_UNSPEC;               /* ipv4/ipv6 agnostic */
        hints.ai_socktype = SOCK_STREAM;           /* TCP stream sockets */
        hints.ai_flags = AI_PASSIVE;               /* ips we can bind to */

        /* set up addrinfo */
        snprintf(tcpport, 5, "%li", config->port);
        status = getaddrinfo(NULL, tcpport, &hints, servinfo);
        if (status != 0) {
                fprintf(stderr, "getaddrinfo error: %s\n",
                        gai_strerror(status));
                _exit(EXIT_FAILURE);
        }

        /* get a socket */
        sock = socket((*servinfo)->ai_family, (*servinfo)->ai_socktype,
                (*servinfo)->ai_protocol);
        if (sock == -1) {
                perror("socket");
                freeaddrinfo(*servinfo);
                _exit(EXIT_FAILURE);
        }

        /* reuse socket if already in use */
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))==-1){
                perror("setsockopt SO_REUSEADDR");
                freeaddrinfo(*servinfo);
                _exit(EXIT_FAILURE);
        }
        return sock;
}

int server_start(int lockfd)
{
        int getaddrinfo(const char *node,
                        const char *service,
                        const struct addrinfo *hints,
                        struct addrinfo **res);

        int errsv;
        int new_fd;
        pid_t pid;
        socklen_t addr_size;
        struct addrinfo *servinfo;
        struct sockaddr_storage their_addr;
        char buf[sizeof(long)];

        if (config->ssl == 1) ssl_setup();

        /* get a socket and bind */
        sockme = get_socket(&servinfo);
        bind(sockme, servinfo->ai_addr, servinfo->ai_addrlen);
        freeaddrinfo(servinfo);

        /* listening */
        if (listen(sockme, BACKLOG) == 0) {
                syslog(LOG_INFO, "Listening on port %li", config->port);
        }
        else {
                errsv = errno;
                fprintf(stderr, "ERROR: %s\n", strerror(errsv));
                syslog(LOG_ERR, "Failed to listen on port %li. Exiting.", 
                                                                config->port);
                free_config();
                exit(EXIT_FAILURE);
        }

        /* drop privileges */
        gid_t newgid = getgid();
        setgroups(1, &newgid);
        if (setuid(getuid()) != 0) {
                fprintf(stderr,
                        "ERROR: Failed to drop root privileges.  Exiting.\n");
                exit(EXIT_FAILURE);
        }
        /* verify privileges cannot be restored */
        if (setuid(0) != -1) {
                fprintf(stderr,
                        "ERROR: Regained root privileges.  Exiting.\n");
                exit(EXIT_FAILURE);
        }

        addr_size = sizeof their_addr;

        /* daemonize */
        if (config->daemon == 0) {
                if (daemon(0, 0) == -1) {
                        errsv = errno;
                        fprintf(stderr, "ERROR: %s\n", strerror(errsv));
                        syslog(LOG_ERR, "Failed to daemonize. Exiting.");
                        free_config();
                        exit(EXIT_FAILURE);
                }
        }

        /* write pid to lockfile */
        snprintf(buf, sizeof(long), "%ld\n", (long) getpid());
        if (write(lockfd, buf, strlen(buf)) != strlen(buf)) {
                fprintf(stderr, "Error writing to pidfile\n");
                exit(EXIT_FAILURE);
        }

        /* install exit handler to kill child procs */
        atexit(killhandlerprocs);

        for (;;) {
                /* incoming! */
                ++hits;
                new_fd = accept(sockme, (struct sockaddr *)&their_addr,
                                &addr_size);
                pid = fork(); /* fork new process to handle connection */
                if (pid == -1) {
                        /* fork failed */
                        return -1;
                }
                else if (pid == 0) {
                        /* let the children play */
                        close(sockme); /* children never listen */
                        handle_connection(new_fd, their_addr);
                }
                else {
                        /* parent can close connection */
                        handler_procs++;
                        close(new_fd);
                }
        }
}

int get_hits()
{
        return hits;
}
