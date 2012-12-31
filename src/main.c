/*
 * gladd - an httpd
 */

#define _GNU_SOURCE

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <syslog.h>
#include <unistd.h>

#include "main.h"
#include "handler.h"

static void sigchld_handler (int signo);
static void sigterm_handler (int signo);

/* clean up zombie children */
static void sigchld_handler (int signo)
{
        int status;
        wait(&status);
}

/* catch SIGTERM and clean up */
static void sigterm_handler (int signo)
{
        close(sockme);
        syslog(LOG_INFO, "Received SIGTERM.  Shutting down.");
        closelog();
        exit(EXIT_SUCCESS);
}

void respond (int fd, char *response)
{
        send(fd, response, strlen(response), 0);
}

int main (void)
{
        int getaddrinfo(const char *node,
                        const char *service,
                        const struct addrinfo *hints,
                        struct addrinfo **res);

        int concounter = 0;
        int lockfd;
        int new_fd;
        int status;
        int yes=1;
        pid_t pid;
        socklen_t addr_size;
        struct addrinfo *servinfo;
        struct addrinfo hints;
        struct sockaddr_storage their_addr;

        /* obtain lockfile */
        lockfd = creat(LOCKFILE, 0644);
        if (lockfd == -1) {
                printf("Failed to open lockfile %s\n", LOCKFILE);
                exit(EXIT_FAILURE);
        }
        if (flock(lockfd, LOCK_EX|LOCK_NB) != 0) {
                printf("%s already running\n", PROGRAM);
                exit(EXIT_FAILURE);
        }

        /* open syslogger */
        openlog(PROGRAM, LOG_CONS|LOG_PID, LOG_DAEMON);

        syslog(LOG_INFO, "Starting up.");

        memset(&hints, 0, sizeof hints);        /* zero memory */
        hints.ai_family = AF_UNSPEC;            /* ipv4/ipv6 agnostic */
        hints.ai_socktype = SOCK_STREAM;        /* TCP stream sockets */
        hints.ai_flags = AI_PASSIVE;            /* get my ip */

        if ((status = getaddrinfo(NULL, "3000", &hints, &servinfo)) != 0) {
                fprintf(stderr, "getaddrinfo error: %s\n",
                                gai_strerror(status));
                exit(EXIT_FAILURE);
        }

        /* get a socket */
        sockme = socket(servinfo->ai_family, servinfo->ai_socktype,
                        servinfo->ai_protocol);

        /* reuse socket if already in use */
        setsockopt(sockme, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        /* bind to a port */
        bind(sockme, servinfo->ai_addr, servinfo->ai_addrlen);

        freeaddrinfo(servinfo);

        /* listening */
        listen(sockme, BACKLOG);

        addr_size = sizeof their_addr;

        daemon(0, 0);

        /* set up child signal handler */
        signal(SIGCHLD, sigchld_handler);

        /* catch SIGTERM for cleanup */
        signal(SIGTERM, sigterm_handler);

        for (;;) {
                /* incoming! */
                ++concounter;
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
                        close(new_fd);
                }
        }
}

