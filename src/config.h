#ifndef __GLADD_CONFIG_H__
#define __GLADD_CONFIG_H__ 1

struct config_t {
        int debug;
        int port;
} config = {
        0,      /* debug */
        3000       /* port */
};

int read_config(char *configfile);
int process_config_line(char *line);

#endif /* __GLADD_CONFIG_H__ */
