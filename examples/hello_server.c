#include <stdio.h>
#include <errno.h>

#define SOCK_IMPLEMENTATION
#include "sock.h"

int main(void) {
    char *err = "None";

    Sock *sock = sock_create(SOCK_IPV4, SOCK_TCP);
    if (sock == NULL) { err = "create"; goto defer; }

    SockAddr addr = sock_addr("0.0.0.0", 6969);
    if (!sock_bind(sock, addr)) { err = "bind"; goto defer; }
    if (!sock_listen(sock, 16)) { err = "listen"; goto defer; }

    Sock *client = sock_accept(sock);
    if (client == NULL) { err = "accept"; goto defer; }

    const char *msg = "Hello from server!";
    char buf[128];
    memset(buf, 0, sizeof(buf));

    sock_send(client, msg, strlen(msg));
    sock_recv(client, buf, sizeof(buf));
    printf("%.*s\n", (int)sizeof(buf), buf);

    sock_close(client);

defer:
    sock_close(sock);
    fprintf(stderr, "ERROR: %s: %s\n", err, strerror(errno));

    return 0;
}
