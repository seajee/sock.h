#include <stdio.h>
#include <errno.h>

#define SOCK_IMPLEMENTATION
#include "sock.h"

int main(void)
{
    bool err = false;

    Sock *s = sock(SOCK_IPV6, SOCK_TCP);
    if (s == NULL) { err = "create"; goto defer; }

    SockAddr addr = sock_addr("::1", 6969);

    if (!sock_connect(s, addr)) { err = "connect"; goto defer; }

    const char *msg = "Hello from client!";
    char buf[128];
    memset(buf, 0, sizeof(buf));

    sock_send(s, msg, strlen(msg));
    sock_recv(s, buf, sizeof(buf));
    printf("%s:%d: %.*s\n", s->addr.str, s->addr.port,
            (int)sizeof(buf), buf);

defer:
    sock_close(s);
    if (err) sock_log_error();

    return 0;
}
