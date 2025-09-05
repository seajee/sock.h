#include <stdio.h>

#define SOCK_IMPLEMENTATION
#include "sock.h"

int main(void)
{
    const char *domain_name = "example.com";
    SockAddrList addr_list = sock_dns(domain_name, 80, SOCK_IPV4, SOCK_TCP);
    if (addr_list.count == 0) {
        fprintf(stderr, "ERROR: Could not resolve address %s\n", domain_name);
        return 1;
    }

    SockAddr addr = addr_list.items[0];
    sock_addr_list_free(&addr_list);

    printf("%s:%d\n", addr.str, addr.port);

    Sock *s = sock_create(addr.type, SOCK_TCP);
    if (s == NULL) {
        fprintf(stderr, "sock_create: ");
        sock_log_error(s);
        return 1;
    }

    if (!sock_connect(s, addr)) {
        fprintf(stderr, "sock_connect: ");
        sock_log_error(s);
        sock_close(s);
        return 1;
    }

    const char *req =
        "GET / HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Connection: close\r\n"
        "\r\n";

    sock_send(s, req, strlen(req));

    char res[1024];
    memset(res, 0, sizeof(res));
    while (sock_recv(s, res, sizeof(res)-1) > 0) {
        printf("%s\n", res);
        memset(res, 0, sizeof(res));
    }

    sock_close(s);

    return 0;
}
