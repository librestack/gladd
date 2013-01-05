#ifndef __GLADD_CONFIG_H__
#define __GLADD_CONFIG_H__ 1

#include <stdio.h>

typedef struct config_t {
        int debug;
        long port;
} config_t;

extern config_t config;

FILE *open_config(char *configfile);
int process_config_line(char *line);
int read_config(char *configfile);

#endif /* __GLADD_CONFIG_H__ */
