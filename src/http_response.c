#include "http_response.h"
#include <stdio.h>
#include <string.h>

int build_hello_response(char *buffer, int size)
{
    const char *body = "Hello, Web!\n";
    int         body_len = (int)strlen(body);
    int         written;

    if (buffer == NULL || size < 0) {
        return -1;
    }

    written = snprintf(
        buffer,
        size,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %d\r\n"
        "\r\n"
        "%s",
        body_len,
        body
    );

    if (written < 0) {
        return -1;
    }
    return 0;
}
