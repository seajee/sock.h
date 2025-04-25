#include <stdio.h>
#include <errno.h>

#define SOCK_IMPLEMENTATION
#include "sock.h"

int main(void)
{
    bool err = false;

    Sock *sock = sock_create(SOCK_IPV6, SOCK_TCP);
    if (sock == NULL) { err = "create"; goto defer; }

    SockAddr addr = sock_addr("::1", 6969);

    if (!sock_connect(sock, addr)) { err = "connect"; goto defer; }

    const char *msg = "Hello from client!";
    char buf[128];
    memset(buf, 0, sizeof(buf));

    sock_send(sock, msg, strlen(msg));
    sock_recv(sock, buf, sizeof(buf));
    printf("%s:%d: %.*s\n", sock->addr.str, sock->addr.port,
            (int)sizeof(buf), buf);

defer:
    sock_close(sock);
    if (err) sock_log_errors();

    return 0;
}
