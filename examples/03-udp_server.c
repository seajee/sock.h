#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SOCK_IMPLEMENTATION
#include "sock.h"

int main(void)
{
    Sock *server = sock_create(SOCK_IPV4, SOCK_UDP);
    if (server == NULL) {
        perror("sock_create");
        return 1;
    }

    SockAddr server_addr = sock_addr("0.0.0.0", 6969);
    if (!sock_bind(server, server_addr)) {
        perror("sock_bind");
        sock_close(server);
        return 1;
    }

    printf("Server is listening on port %d...\n", server_addr.port);

    SockAddr client_addr;
    char buf[64];

    while (true) {
        ssize_t rec = sock_recvfrom(server, buf, sizeof(buf), &client_addr);
        if (rec < 0) {
            perror("sock_recvfrom");
            break;
        }

        buf[rec] = '\0';
        printf("Received from %s:%d \"%s\"\n", client_addr.str,
                client_addr.port, buf);
    }

    sock_close(server);
    return 0;
}
