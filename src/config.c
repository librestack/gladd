#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"

/* set config defaults */
config_t config_default = {
        .debug          = 0,
        .port           = 8080
};

config_t *config;
config_t *config_new;

url_t *prevurl;        /* pointer to last url */

/* set config defaults if they haven't been set already */
int set_config_defaults()
{
        static int defaults_set = 0;

        if (defaults_set != 0)
                return 1;

        config = &config_default;

        defaults_set = 1;
        return 0;
}

/* set config value if long integer is between min and max */
int set_config_long(long *confset, char *keyname, long i, long min, long max)
{
        if ((i >= min) && (i <= max)) {
                *confset = i;
        }
        else {
                fprintf(stderr,"ERROR: invalid %s value\n", keyname);
                return -1;
        }
        return 0;
}

/* clean up config->urls memory */
void free_urls()
{
        url_t *u;
        url_t *tmp;

        u = config->urls;
        while (u != NULL) {
                free(u->url);
                free(u->path);
                tmp = u;
                u = u->next;
                free(tmp);
        }
}

/* add url handler */
int add_url_handler(char *value)
{
        url_t *newurl;
        char type[8];
        char url[LINE_MAX];
        char path[LINE_MAX];
        char params[LINE_MAX];

        newurl = malloc(sizeof(struct url_t));

        if (sscanf(value, "%s %[^\n]", type, params) == 2) {
                if (strncmp(type, "static", 6) == 0) {
                        /* static url handler */
                        if (sscanf(params, "%s %s", url, path) == 2) {
                                newurl->type = "static";
                                newurl->url = strdup(url);
                                newurl->path = strdup(path);
                                newurl->next = NULL;
                                if (prevurl != NULL) {
                                        /* update ->next ptr in previous url
                                         * to point to new */
                                        prevurl->next = newurl;
                                }
                                else {
                                        /* no previous url, 
                                         * so set first ptr in config */
                                        config_new->urls = newurl;
                                }
                                prevurl = newurl;
                        }
                }
        }

        /* TODO: plug this leak - can't free() here, need to do later */
        //free(newurl);

        /* check newurl-> url etc are free()'d */

        return 0;
}

/* check config line and handle appropriately */
int process_config_line(char *line)
{
        long i = 0;
        char key[LINE_MAX] = "";
        char value[LINE_MAX] = "";

        if (line[0] == '#')
                return 1; /* skipping comment */
        
        if (sscanf(line, "%[a-zA-Z0-9]", value) == 0) {
                return 1; /* skipping blank line */
        }
        else if (sscanf(line, "%s %li", key, &i) == 2) {
                /* process long integer config values */
                if (strncmp(key, "debug", 5) == 0) {
                        return set_config_long(&config_new->debug,
                                                "debug", i, 0, 1);
                }
                else if (strncmp(key, "port", 4) == 0) {
                        return set_config_long(&config_new->port, 
                                                "port", i, 1, 65535);
                }
        }
        else if (sscanf(line, "%s %[^\n]", key, value) == 2) {
                if (strncmp(key, "url", 3) == 0) {
                        return add_url_handler(value);
                }
        }

        return -1; /* syntax error */
}

/* open config file for reading */
FILE *open_config(char *configfile)
{
        FILE *fd;

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
        int retval = 0;

        set_config_defaults();
        config_new = &config_default;

        prevurl = NULL;

        /* open file for reading */
        fd = open_config(configfile);
        if (fd == NULL)
                return 1;

        /* read in config */
        while (fgets(line, LINE_MAX, fd) != NULL) {
                lc++;
                if (process_config_line(line) < 0) {
                        printf("Error in line %i of %s.\n", lc, configfile);
                        retval = 1;
                        break;
                }
        }

        /* close file */
        fclose(fd);

        /* if config parsed okay, make active */
        if (retval == 0)
                config = config_new;

        return retval;
}

