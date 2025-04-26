#include <stdio.h>
#include <string.h>

#define SOCK_IMPLEMENTATION
#include "sock.h"

int main(void)
{
    Sock *client = sock(SOCK_IPV4, SOCK_UDP);
    if (client == NULL) {
        perror("sock_create");
        return 1;
    }

    SockAddr server_addr = sock_addr("127.0.0.1", 6969);
    const char *msg = "Hello from client!";
    ssize_t sent = sock_sendto(client, msg, strlen(msg), server_addr);
    if (sent < 0) {
        perror("sock_sendto");
        sock_close(client);
        return 1;
    }

    printf("Sent message: %s\n", msg);

    sock_close(client);
    return 0;
}
