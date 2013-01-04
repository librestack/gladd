#ifndef __GLADD_CONFIG_H__
#define __GLADD_CONFIG_H__ 1

typedef struct config_t {
        int debug;
        int port;
} config_t;

extern config_t config;

int read_config(char *configfile);
int process_config_line(char *line);

#endif /* __GLADD_CONFIG_H__ */
