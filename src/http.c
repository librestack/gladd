#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "http.h"

struct http_status httpcode[] = {
        { 200, "OK" },
        { 400, "Bad Request" },
        { 404, "Not Found" },
        { 405, "Method Not Allowed" }
};

void http_response(int sock, int code)
{
        char *response;
        char *status;
        char *mime = "text/html";

        status = get_status(code).status;
        asprintf(&response, HTTP_RESPONSE, code, status, mime, code, status);
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

