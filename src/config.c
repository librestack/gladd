#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"

/* set config defaults */
config_t config = {
        .debug = 0,
        .port = 8080
};

/* check config line and handle appropriately */
int process_config_line(char *line)
{
        long i = 0;
        char str[LINE_MAX] = "";

        if (line[0] == '#')
                return 1; /* skipping comment */
        
        if (sscanf(line, "%[a-zA-Z0-9]", str) == 0) {
                /* skipping blank line */
                return 1;
        }
        else if (sscanf(line, "debug %li", &i) == 1) {
                if ((i >= 0) && (i <= 1)) {
                        config.debug = i;
                }
                else {
                        fprintf(stderr, "ERROR: invalid debug value\n");
                        return -1;
                }
                return 0;
        }
        else if (sscanf(line, "port %li", &i) == 1) {
                if ((i > 0) && (i <= 65535)) {
                        config.port = i;
                }
                else {
                        fprintf(stderr, "ERROR: invalid port\n");
                        return -1;
                }
                return 0;
        }

        fprintf(stderr, "ERROR: unrecognised key\n");
        return -1; /* unrecognised key */
}

FILE *open_config(char *configfile)
{
        FILE *fd;

        /* open file for reading */
        fd = fopen(configfile, "r");
        if (fd == NULL) {
                int errsv = errno;
                fprintf(stderr, "ERROR: %s\n", strerror(errsv));
        }
        return fd;
}

/* read config file into memory */
int read_config(char *configfile)
{
        FILE *fd;
        char line[LINE_MAX];
        int lc = 0;

        /* open file for reading */
        fd = open_config(configfile);
        if (fd == NULL)
                return 1;

        /* read in config */
        while (fgets(line, LINE_MAX, fd) != NULL) {
                lc++;
                if (process_config_line(line) < 0) {
                        printf("Error in line %i of %s.\n", lc, configfile);
                        return 1;
                }
        }

        /* close file */
        fclose(fd);

        return 0;
}

