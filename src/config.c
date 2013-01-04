#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

/* check config line and handle appropriately */
int process_config_line(char *line)
{
        if (line[0] == '#')
                return 1; /* skipping comment */
        


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

