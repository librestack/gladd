#ifndef __GLADD_HTTP_H__
#define __GLADD_HTTP_H__ 1

#define HTTPKEYS (sizeof httpcode / sizeof (struct http_status))
#define HTTP_RESPONSE "HTTP/1.1 %i %s\nServer: gladd\nConnection: close\nContent-Type: %s\n\n<html><body><h1>%i %s</h1>\n</body>\n</html>\n"

struct http_status {
        int code;
        char *status;
};

struct http_status get_status(int code);
void http_response(int sock, int code);

#endif /* __GLADD_HTTP_H__ */
