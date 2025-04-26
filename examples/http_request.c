#include <stdio.h>

#define SOCK_IMPLEMENTATION
#include "sock.h"

int main(void)
{
    SockAddr addr = sock_addr("example.com", 80);
    if (addr.type == SOCK_ADDR_INVALID) {
        sock_log_error();
        return 1;
    }

    printf("%s:%d\n", addr.str, addr.port);

    Sock *sock = sock_create(addr.type, SOCK_TCP);
    if (sock == NULL) {
        fprintf(stderr, "sock_create: ");
        sock_log_error();
        return 1;
    }

    if (!sock_connect(sock, addr)) {
        fprintf(stderr, "sock_connect: ");
        sock_log_error();
        sock_close(sock);
        return 1;
    }

    const char *req =
        "GET / HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Connection: close\r\n\r\n";

    sock_send(sock, req, strlen(req));

    char res[1024];
    memset(res, 0, sizeof(res));
    while (sock_recv(sock, res, sizeof(res)-1) > 0) {
        printf("%s\n", res);
        memset(res, 0, sizeof(res));
    }

    sock_close(sock);

    return 0;
}
