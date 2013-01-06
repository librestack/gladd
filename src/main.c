/*
 * gladd - an httpd
 */

#define _GNU_SOURCE

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <syslog.h>
#include <unistd.h>

#include "main.h"
#include "config.h"
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

/* catch HUP and reload config */
static void sighup_handler (int signo)
{
        syslog(LOG_INFO, "Received SIGHUP.  Reloading config.");

        if (read_config(DEFAULT_CONFIG) != 0) {
                syslog(LOG_ERR, "Config reload failed.");
        }

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
        int errsv;
        int lockfd;
        int new_fd;
        int status;
        int yes=1;
        pid_t pid;
        socklen_t addr_size;
        struct addrinfo *servinfo;
        struct addrinfo hints;
        struct sockaddr_storage their_addr;
        char tcpport[5];
        char *errmsg;

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

        /* read config */
        if (read_config(DEFAULT_CONFIG) != 0) {
                asprintf(&errmsg,"Failed to read config on startup. Exiting.");
                syslog(LOG_ERR, "%s", errmsg);
                fprintf(stderr, "%s\n", errmsg);
                free(errmsg);
                exit(EXIT_FAILURE);
        }

        memset(&hints, 0, sizeof hints);           /* zero memory */
        hints.ai_family = AF_UNSPEC;               /* ipv4/ipv6 agnostic */
        hints.ai_socktype = SOCK_STREAM;           /* TCP stream sockets */
        hints.ai_flags = AI_PASSIVE;               /* get my ip */
        snprintf(tcpport, 5, "%li", config->port); /* tcp port to listen on */

        if ((status = getaddrinfo(NULL, tcpport, &hints, &servinfo)) != 0){
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
        if (listen(sockme, BACKLOG) == 0) {
                syslog(LOG_INFO, "Listening on port %li", config->port);
        }
        else {
                errsv = errno;
                fprintf(stderr, "ERROR: %s\n", strerror(errsv));
                syslog(LOG_ERR, "Failed to listen on port %li. Exiting.", 
                                                                config->port);
                exit(EXIT_FAILURE);
        }

        addr_size = sizeof their_addr;

        daemon(0, 0);

        /* set up child signal handler */
        signal(SIGCHLD, sigchld_handler);

        /* catch SIGTERM for cleanup */
        signal(SIGTERM, sigterm_handler);

        /* catch HUP signal for config reload */
        signal(SIGHUP, sighup_handler);

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

