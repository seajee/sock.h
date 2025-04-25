// sock - v1.0.2 - MIT License - https://github.com/seajee/sock.h

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
    SockAddrType type;                // Address type
    int port;                         // Address port
    char str[SOCK_ADDR_STR_CAPACITY]; // String representation of the address
    union {
        struct sockaddr sockaddr;
        struct sockaddr_in ipv4;
        struct sockaddr_in6 ipv6;
    };
    socklen_t len;
} SockAddr;

typedef enum {
    SOCK_TCP,
    SOCK_UDP
} SockType;

typedef struct {
    SockType type;  // Socket type
    SockAddr addr;  // Socket address
    int fd;         // File descriptor
} Sock;

// Create a SockAddr structure from primitives
SockAddr sock_addr(const char *addr, int port);

// Create a socket with the corresponding domain and type
Sock *sock_create(SockAddrType domain, SockType type);

// Bind a socket to a specific address
bool sock_bind(Sock *sock, SockAddr addr);

// Make the socket listen to incoming connections
bool sock_listen(Sock *sock, int backlog);

// Accept connections from a socket
Sock *sock_accept(Sock *sock);

// Connect a socket to a specific address
bool sock_connect(Sock *sock, SockAddr addr);

// Send data through a socket
ssize_t sock_send(Sock *sock, const void *buf, size_t size);

// Receive data from a socket
ssize_t sock_recv(Sock *sock, void *buf, size_t size);

// Send data through a socket in `connectionless` mode
ssize_t sock_sendto(Sock *sock, const void *buf, size_t size, SockAddr addr);

// Receive data from a socket in `connectionless` mode
ssize_t sock_recvfrom(Sock *sock, void *buf, size_t size, SockAddr *addr);

// Close a socket
void sock_close(Sock *sock);

#endif // SOCK_H_

#ifdef SOCK_IMPLEMENTATION

SockAddr sock_addr(const char *addr, int port)
{
    SockAddr sa = {0};

    if (inet_pton(AF_INET, addr, &sa.sockaddr) == 1) {
        sa.type = SOCK_IPV4;
        sa.len = sizeof(sa.ipv4);
        sa.ipv4.sin_family = AF_INET;
        sa.ipv4.sin_port = htons(port);
    } else if (inet_pton(AF_INET6, addr, &sa.sockaddr) == 1) {
        sa.type = SOCK_IPV6;
        sa.len = sizeof(sa.ipv6);
        sa.ipv6.sin6_family = AF_INET6;
        sa.ipv6.sin6_port = htons(port);
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
                           &sock->addr.len);
    if (res < 0) {
        return res;
    }

    printf("Received sa_family: %d\n", addr->sockaddr.sa_family);

    switch (addr->sockaddr.sa_family) {
        case AF_INET: {
            struct sockaddr_in *ipv4 = &addr->ipv4;
            addr->type = SOCK_IPV4;
            addr->port = ntohs(ipv4->sin_port);
            addr->len = sizeof(*ipv4);
            inet_ntop(AF_INET, &ipv4->sin_addr, addr->str, sizeof(addr->str));
        } break;

        case AF_INET6: {
            struct sockaddr_in6 *ipv6 = &addr->ipv6;
            addr->type = SOCK_IPV6;
            addr->port = ntohs(ipv6->sin6_port);
            addr->len = sizeof(*ipv6);
            inet_ntop(AF_INET6, &ipv6->sin6_addr, addr->str, sizeof(addr->str));
        } break;

        default: {
            assert(false && "Unreachable address family");
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

/*
    Revision history:

        1.0.2 (2025-04-25) Fix sock_recvfrom not recognizing the address
                           family
        1.0.1 (2025-04-25) Handle different kind of sockaddr with a union
        1.0.0 (2025-04-25) Initial release
*/

/*
 * MIT License
 * 
 * Copyright (c) 2025 seajee
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
