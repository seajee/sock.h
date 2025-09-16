#include <assert.h>

#define SOCK_IMPLEMENTATION
#include "sock.h"

void send_len(Sock *s, const void *buf, size_t size)
{
    bool ok = true;

    // First send size of msg
    ok = sock_send_all(s, &size, sizeof(size));
    assert(ok);

    // Then send msg
    ok = sock_send_all(s, buf, size);
    assert(ok);
}

void server()
{
    bool ok = true;

    Sock *server = sock_create(SOCK_IPV4, SOCK_TCP);
    assert(server != NULL);

    ok = sock_bind(server, sock_addr("0.0.0.0", 6969));
    assert(ok);

    ok = sock_listen(server);
    assert(ok);

    Sock *client = sock_accept(server);
    assert(client != NULL);

    ssize_t n = 0;

    size_t len = 0;
    n = sock_recv_all(client, &len, sizeof(len));
    assert(n >= 0);

    char buf[len + 1];
    memset(buf, 0, len + 1);

    n = sock_recv_all(client, buf, len);
    assert(n >= 0);
    printf("%.*s\n", (int)len, buf);

    const char *msg = "Hello from Server";
    send_len(client, msg, strlen(msg));

    sock_close(client);
    sock_close(server);
}

void client()
{
    bool ok = true;

    Sock *sock = sock_create(SOCK_IPV4, SOCK_TCP);
    assert(sock != NULL);

    ok = sock_connect(sock, sock_addr("127.0.0.1", 6969));
    assert(ok);

    const char *msg = "Hello from Client";
    send_len(sock, msg, strlen(msg));

    ssize_t n = 0;

    size_t len = 0;
    n = sock_recv_all(sock, &len, sizeof(len));
    assert(n >= 0);

    char buf[len + 1];
    memset(buf, 0, len + 1);

    n = sock_recv_all(sock, buf, len);
    assert(n >= 0);
    printf("%.*s\n", (int)len, buf);

    sock_close(sock);
}

int main(int argc, char **argv)
{
    (void) argv;

    if (argc > 1) {
        server();
    } else {
        client();
    }

    return 0;
}
