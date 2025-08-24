#include <stdio.h>

#define SOCK_IMPLEMENTATION
#include "sock.h"

int main(void)
{
    bool err = false;

    Sock *server = sock_create(SOCK_IPV6, SOCK_UDP);
    if (server == NULL) { err = true; goto close; }

    SockAddr addr = sock_addr("::", 6969);
    if (!sock_bind(server, addr)) { err = true; goto close; }

    char buf[128];
    memset(buf, 0, sizeof(buf));
    const char *msg = "Hello from server!";
    SockAddr client_addr = {0};

    sock_recvfrom(server, buf, sizeof(buf), &client_addr);
    sock_sendto(server, msg, strlen(msg), client_addr);

    printf("%s:%d: %.*s\n", client_addr.str, client_addr.port,
            (int)sizeof(buf), buf);

close:
    if (err) sock_log_error(server);
    sock_close(server);

    return 0;
}
