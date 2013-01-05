#ifndef __GLADD_CONFIG_H__
#define __GLADD_CONFIG_H__ 1

#include <stdio.h>

typedef struct config_t {
        long debug;
        long port;
} config_t;

extern config_t config;

FILE *open_config(char *configfile);
int process_config_line(char *line);
int read_config(char *configfile);
int set_config_defaults();
int set_config_long(long *confset, char *keyname, long i, long min, long max);

#endif /* __GLADD_CONFIG_H__ */
