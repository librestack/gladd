#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

/* recursively create directory and parents */
int rmkdir(char *dir, mode_t mode)
{
        char *parent;
        int i;
        int r;

        if (mkdir(dir, mode) != 0) {
                if (errno == ENOENT) {
                        parent = strdup(dir);
                        for (i=strlen(parent);i>0;--i) {
                                if (parent[i] == '/') {
                                        parent[i] = '\0';
                                        if ((r=rmkdir(parent,mode))!= 1) break;
                                }
                        }
                        free(parent);
                        if (r == 0) return 0;
                        rmkdir(dir, mode);
                }
                else if (errno != EEXIST) {
                        syslog(LOG_ERR, "Error creating directory '%s': %s",
                                dir, strerror(errno));
                        return 0;
                }
                else {
                        return -1;
                }
        }
        else {
                syslog(LOG_DEBUG, "Directory '%s' created", dir);
                return 1;
        }
        return -1;
}
