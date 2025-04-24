#ifndef SOCK_H_
#define SOCK_H_

#include <arpa/inet.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SOCK_ADDR_STR_CAPACITY 64

typedef enum {
    SOCK_ADDR_INVALID,
    SOCK_IPV4,
    SOCK_IPV6
} SockAddrType;

typedef struct {
    SockAddrType type;
    int port;
    struct sockaddr sockaddr;
    socklen_t len;
    char str[SOCK_ADDR_STR_CAPACITY];
} SockAddr;

typedef enum {
    SOCK_TCP,
    SOCK_UDP
} SockType;

typedef struct {
    SockType type;
    SockAddr addr;
    int fd;
} Sock;

SockAddr sock_addr(const char *addr, int port);

Sock *sock_create(SockAddrType domain, SockType type);
bool sock_bind(Sock *sock, SockAddr addr);
bool sock_listen(Sock *sock, int backlog);
Sock *sock_accept(Sock *sock);
bool sock_connect(Sock *sock, SockAddr addr);
ssize_t sock_send(Sock *sock, const void *buf, size_t size);
ssize_t sock_recv(Sock *sock, void *buf, size_t size);
ssize_t sock_sendto(Sock *sock, const void *buf, size_t size, SockAddr addr);
ssize_t sock_recvfrom(Sock *sock, void *buf, size_t size, SockAddr *addr);
void sock_close(Sock *sock);

#endif // SOCK_H_

#ifdef SOCK_IMPLEMENTATION

SockAddr sock_addr(const char *addr, int port)
{
    SockAddr sa = {0};

    if (inet_pton(AF_INET, addr, &sa.sockaddr) == 1) {
        sa.type = SOCK_IPV4;
        sa.len = sizeof(struct sockaddr_in);
        struct sockaddr_in *in = (struct sockaddr_in*)&sa.sockaddr;
        in->sin_family = AF_INET;
        in->sin_port = htons(port);
    } else if (inet_pton(AF_INET6, addr, &sa.sockaddr) == 1) {
        sa.type = SOCK_IPV6;
        sa.len = sizeof(struct sockaddr_in6);
        struct sockaddr_in6 *in6 = (struct sockaddr_in6*)&sa.sockaddr;
        in6->sin6_family = AF_INET6;
        in6->sin6_port = htons(port);
    } else {
        sa.type = SOCK_ADDR_INVALID;
        return sa;
    }

    sa.port = port;
    strncpy(sa.str, addr, sizeof(sa.str));

    return sa;
}

Sock *sock_create(SockAddrType domain, SockType type)
{
    Sock *sock = malloc(sizeof(*sock));
    if (sock == NULL) {
        return NULL;
    }

    memset(sock, 0, sizeof(*sock));

    sock->type = type;

    int sock_domain = 0;
    int sock_type = 0;

    switch (domain) {
        case SOCK_IPV4: sock_domain = AF_INET; break;
        case SOCK_IPV6: sock_domain = AF_INET6; break;
        default: {
            free(sock);
            return NULL;
        } break;
    }

    switch (type) {
        case SOCK_TCP: sock_type = SOCK_STREAM; break;
        case SOCK_UDP: sock_type = SOCK_DGRAM; break;
        default: {
            free(sock);
            return NULL;
        } break;
    }

    sock->fd = socket(sock_domain, sock_type, 0);
    if (sock->fd < 0) {
        free(sock);
        return NULL;
    }

    return sock;
}

bool sock_bind(Sock *sock, SockAddr addr)
{
    if (bind(sock->fd, &addr.sockaddr, addr.len) < 0) {
        return false;
    }

    sock->addr = addr;

    return true;
}

bool sock_listen(Sock *sock, int backlog)
{
    if (listen(sock->fd, backlog) < 0) {
        return false;
    }

    return true;
}

Sock *sock_accept(Sock *sock)
{
    int fd = accept(sock->fd, &sock->addr.sockaddr, &sock->addr.len);
    if (fd < 0) {
        return NULL;
    }

    Sock *res = malloc(sizeof(*res));
    if (res == NULL) {
        return NULL;
    }

    memset(res, 0, sizeof(*res));

    res->type = sock->type;
    res->fd = fd;

    return res;
}

bool sock_connect(Sock *sock, SockAddr addr)
{
    if (connect(sock->fd, &addr.sockaddr, addr.len) < 0) {
        return false;
    }

    return true;
}

ssize_t sock_send(Sock *sock, const void *buf, size_t size)
{
    return send(sock->fd, buf, size, 0);
}

ssize_t sock_recv(Sock *sock, void *buf, size_t size)
{
    return recv(sock->fd, buf, size, 0);
}

ssize_t sock_sendto(Sock *sock, const void *buf, size_t size, SockAddr addr)
{
    return sendto(sock->fd, buf, size, 0, &addr.sockaddr, addr.len);
}

ssize_t sock_recvfrom(Sock *sock, void *buf, size_t size, SockAddr *addr)
{
    ssize_t res = recvfrom(sock->fd, buf, size, 0, &addr->sockaddr,
                           &addr->len);

    switch (addr->sockaddr.sa_family) {
        case AF_INET: {
            struct sockaddr_in *sa = (struct sockaddr_in*)&addr->sockaddr;
            addr->type = SOCK_IPV4;
            addr->port = ntohs(sa->sin_port);
            addr->len = sizeof(*sa);
            inet_ntop(AF_INET, &sa->sin_addr, addr->str, sizeof(addr->str));
        } break;

        case AF_INET6: {
            struct sockaddr_in6 *sa = (struct sockaddr_in6*)&addr->sockaddr;
            addr->type = SOCK_IPV6;
            addr->port = ntohs(sa->sin6_port);
            addr->len = sizeof(*sa);
            inet_ntop(AF_INET6, &sa->sin6_addr, addr->str, sizeof(addr->str));
        } break;

        default: {
            assert(false && "Unsupported address family");
        } break;
    }

    return res;
}

void sock_close(Sock *sock)
{
    close(sock->fd);
    free(sock);
}

#endif // SOCK_IMPLEMENTATION
