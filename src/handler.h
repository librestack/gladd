#ifndef __GLADD_HANDLER_H__
#define __GLADD_HANDLER_H__ 1


/* TEMP */
#define RESPONSE_200 "HTTP/1.1 200 OK\nServer: webguessd\nConnection: close\nContent-Type: %s\n\n%s"

#include <sys/socket.h>

void *get_in_addr(struct sockaddr *sa);
void handle_connection(int sock, struct sockaddr_storage their_addr);
int prepare_response(char resource[], char **xml, char *query);
int send_file(int sock, char *path);

#endif /* __GLADD_HANDLER_H__ */
