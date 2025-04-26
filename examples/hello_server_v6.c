#include <stdio.h>

#define SOCK_IMPLEMENTATION
#include "sock.h"

int main(void)
{
    bool err = false;

    Sock *server = sock(SOCK_IPV6, SOCK_TCP);
    if (server == NULL) { err = true; goto close; }

    SockAddr addr = sock_addr("::", 6969);
    if (!sock_bind(server, addr)) { err = true; goto close; }
    if (!sock_listen(server, 16)) { err = true; goto close; }

    Sock *client = sock_accept(server);
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
    sock_close(server);
    if (err) sock_log_error();

    return 0;
}
