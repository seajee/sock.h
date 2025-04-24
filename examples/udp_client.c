#include <stdio.h>

#define SOCK_IMPLEMENTATION
#include "sock.h"

int main(void)
{
    Sock *sock = sock_create(SOCK_IPV4, SOCK_UDP);
    if (sock == NULL) {
        perror("sock_create");
        return 1;
    }

    const char *msg = "Hello from client!";
    sock_sendto(sock, msg, strlen(msg), sock_addr("127.0.0.1", 6969));

    SockAddr server_addr;
    char buf[128];
    memset(buf, 0, sizeof(buf));

    sock_recvfrom(sock, buf, sizeof(buf), &server_addr);

    printf("Received \"%.*s\" from %s:%d\n", (int)sizeof(buf), buf,
            server_addr.str, server_addr.port);

    return 0;
}
