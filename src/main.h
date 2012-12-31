#define _GNU_SOURCE

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <libxml/parser.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syslog.h>
#include <unistd.h>

#define BACKLOG 10  /* how many pending connectiong to hold in queue */
#define BUFSIZE 8096
#define LOCKFILE ".gladd.lock"
#define MIME_DEFAULT "application/octet-stream"
#define MIME_XML "application/xml"
#define PROGRAM "gladd"

#define RESPONSE_200 "HTTP/1.1 200 OK\nServer: gladd\nConnection: close\nContent-Type: %s\n\n%s"
#define RESPONSE_405 "HTTP/1.1 405 Method Not Allowed\nServer: gladd\nConnection: close\nContent-Type: text/html\n\n<html><body><h1>405 Method Not Allowed</h1>\n</body>\n</html>\n"

int sockme;

int main (void);
void get_mime_type(char *mimetype, char *filename);
void handle_connection(int sock, struct sockaddr_storage their_addr);
void respond (int fd, char *response);
