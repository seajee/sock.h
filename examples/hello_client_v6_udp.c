#include <stdio.h>
#include <errno.h>

#define SOCK_IMPLEMENTATION
#include "sock.h"

int main(void)
{
    bool err = false;

    Sock *s = sock_create(SOCK_IPV6, SOCK_UDP);
    if (s == NULL) { err = "create"; goto defer; }

    SockAddr addr = sock_addr("::1", 6969);

    const char *msg = "Hello from client!";
    char buf[128];
    memset(buf, 0, sizeof(buf));

    sock_sendto(s, msg, strlen(msg), addr);
    sock_recvfrom(s, buf, sizeof(buf), NULL);

    printf("%s:%d: %.*s\n", addr.str, addr.port,
            (int)sizeof(buf), buf);

defer:
    if (err) sock_log_error(s);
    sock_close(s);

    return 0;
}
