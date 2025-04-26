#include <stdio.h>

#define SOCK_IMPLEMENTATION
#include "sock.h"

int main(void)
{
    bool err = false;

    Sock *sock = sock_create(SOCK_IPV6, SOCK_TCP);
    if (sock == NULL) { err = true; goto close; }

    SockAddr addr = sock_addr("::", 6969);
    if (!sock_bind(sock, addr)) { err = true; goto close; }
    if (!sock_listen(sock, 16)) { err = true; goto close; }

    Sock *client = sock_accept(sock);
    if (client == NULL) { err = true; goto close; }

    const char *msg = "Hello from server!";
    char buf[128];
    memset(buf, 0, sizeof(buf));

    sock_send(client, msg, strlen(msg));
    sock_recv(client, buf, sizeof(buf));
    printf("%s:%d: %.*s\n", client->addr.str, client->addr.port,
            (int)sizeof(buf), buf);

    sock_close(client);

close:
    sock_close(sock);
    if (err) sock_log_error();

    return 0;
}
