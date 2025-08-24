#include <stdio.h>
#include <errno.h>

#define SOCK_IMPLEMENTATION
#include "sock.h"

int main(void)
{
    char *err = "None";

    Sock *socket = sock_create(SOCK_IPV4, SOCK_TCP);
    if (socket == NULL) { err = "create"; goto defer; }

    SockAddr addr = sock_addr("127.0.0.1", 6969);

    if (!sock_connect(socket, addr)) { err = "connect"; goto defer; }

    const char *msg = "Hello from client!";
    char buf[128];
    memset(buf, 0, sizeof(buf));

    sock_send(socket, msg, strlen(msg));
    sock_recv(socket, buf, sizeof(buf));
    printf("%.*s\n", (int)sizeof(buf), buf);

defer:
    sock_close(socket);
    fprintf(stderr, "ERROR: %s: %s\n", err, strerror(errno));

    return 0;
}
