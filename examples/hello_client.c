#include <stdio.h>
#include <errno.h>

#define SOCK_IMPLEMENTATION
#include "sock.h"

int main(void)
{
    char *err = "None";

    Sock *sock = sock_create(SOCK_IPV4, SOCK_TCP);
    if (sock == NULL) { err = "create"; goto defer; }
    printf("create\n");

    SockAddr addr = sock_addr("127.0.0.1", 6969);

    if (!sock_connect(sock, addr)) { err = "connect"; goto defer; }
    printf("connect\n");

    const char *msg = "Hello from client!";
    char buf[128];
    memset(buf, 0, sizeof(buf));

    sock_send(sock, msg, strlen(msg));
    printf("send\n");
    sock_recv(sock, buf, sizeof(buf));
    printf("recv\n");
    printf("%.*s\n", (int)sizeof(buf), buf);

defer:
    sock_close(sock);
    printf("close\n");
    fprintf(stderr, "ERROR: %s: %s\n", err, strerror(errno));

    return 0;
}
