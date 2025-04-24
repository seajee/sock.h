#include <stdio.h>

#define SOCK_IMPLEMENTATION
#include "sock.h"

int main(void)
{
    Sock *server = sock_create(SOCK_IPV4, SOCK_UDP);
    if (server == NULL) {
        perror("sock_create");
        return 1;
    }

    if (!sock_bind(server, sock_addr("0.0.0.0", 6969))) {
        perror("sock_bind");
        sock_close(server);
        return 1;
    }

    SockAddr client_addr;
    char buf[128];
    memset(buf, 0, sizeof(buf));

    sock_recvfrom(server, buf, sizeof(buf), &client_addr);

    printf("Received \"%.*s\" from %s:%d\n", (int)sizeof(buf), buf,
            client_addr.str, client_addr.port);

    const char *msg = "Hello from server!";
    sock_sendto(server, msg, strlen(msg), client_addr);

    sock_close(server);

    return 0;
}
