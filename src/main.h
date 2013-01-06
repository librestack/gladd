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
