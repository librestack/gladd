#define HTTPKEYS (sizeof httpcode / sizeof (struct http_status))
#define HTTP_RESPONSE "HTTP/1.1 %i %s\nServer: gladd\nConnection: close\nContent-Type: text/html\n\n<html><body><h1>%i %s</h1>\n</body>\n</html>\n"

struct http_status {
        int code;
        char *status;
} httpcode[] = {
        { 200, "OK" },
        { 404, "Not Found" },
        { 405, "Method Not Allowed" }
};

struct http_status get_status(int code);
