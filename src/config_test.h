#ifndef __GLADD_CONFIG_TEST_H__
#define __GLADD_CONFIG_TEST_H__ 1

char *test_config_skip_comment();
char *test_config_skip_blank();
char *test_config_invalid_line();
char *test_config_open_success();
char *test_config_open_fail();
char *test_config_default_debug_value();
char *test_config_default_port_value();
char *test_config_set_debug_value();
char *test_config_set_port_value();

#endif /* __GLADD_CONFIG_TEST_H__ */
