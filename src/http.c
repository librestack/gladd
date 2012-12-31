#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "http.h"

void http_response(int sock, int code)
{
        char *response;
        char *status;

        status = get_status(code).status;
        asprintf(&response, HTTP_RESPONSE, code, status, code, status);
        respond(sock, response);
        free(response);
}

struct http_status get_status(int code)
{
        int x;
        struct http_status hcode;

        for (x = 0; x < HTTPKEYS; x++) {
                if (httpcode[x].code == code)
                        hcode = httpcode[x];
        }

        return hcode;
}

