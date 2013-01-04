#include <errno.h>
#include <limits.h>
#include <stdio.h>
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
        char *key = 0;
        char *value = 0;
        //int i;

        if (line[0] == '#')
                return 1; /* skipping comment */
        
        if (snprintf(line, LINE_MAX, "%s %s\n", key, value) < 0)
                return 2; /* line was not valid */
        /*
        if (strncmp(key, "port", 4)) {
                i = atoi(value);
                if ((i > 0) && (i <= 65535))
                        config.port = i;
                else
                        fprintf(stderr, "ERROR: invalid port\n");
                        return 4;
                return 0;
        }
        */

        return 0;
}

/* read config file into memory */
int read_config(char *configfile)
{
        FILE *fd;
        char line[LINE_MAX];

        /* open file for reading */
        fd = fopen(configfile, "r");
        if (fd == NULL) {
                int errsv = errno;
                fprintf(stderr, "ERROR: %s\n", strerror(errsv));
                return 1;
        }
                                                        
        /* read in config */
        while (fgets(line, LINE_MAX, fd) != NULL) {
                process_config_line(line);
        }

        /* close file */
        fclose(fd);

        return 0;
}

