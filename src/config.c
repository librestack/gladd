#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>


int read_config(char *configfile)
{
        FILE *fd;

        /* open file for reading */
        fd = fopen(configfile, "r");
        if (fd == NULL) {
                int errsv = errno;
                fprintf(stderr, "ERROR: %s\n", strerror(errsv));
                return 1;
        }
                                                        
        /* read in config */
        // int fscanf(FILE *stream, const char *format, ...);

        /* close file */
        fclose(fd);

        return 0;
}
