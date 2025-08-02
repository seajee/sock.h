// sock.h - v1.6.1 - MIT License - https://github.com/seajee/sock.h

#ifndef SOCK_H_
#define SOCK_H_

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" { // Prevent name mangling
#endif // __cplusplus

typedef enum {
    SOCK_ADDR_INVALID,
    SOCK_IPV4,
    SOCK_IPV6
} SockAddrType;

typedef struct {
    SockAddrType type;          // Address type
    int port;                   // Address port
    char str[INET6_ADDRSTRLEN]; // String representation of the address
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
    SockType type;    // Socket type
    SockAddr addr;    // Socket address
    int fd;           // File descriptor
    char *last_error; // Last error about this socket
} Sock;

typedef void (*SockThreadFn)(Sock *sock, void *user_data);

typedef struct {
    SockThreadFn fn;
    Sock *sock;
    void *user_data;
} SockThreadData;

// Create a socket with the corresponding domain and type
Sock *sock_create(SockAddrType domain, SockType type);

// Create a SockAddr structure from primitives
SockAddr sock_addr(const char *addr, int port);

// Bind a socket to a specific address
bool sock_bind(Sock *sock, SockAddr addr);

// Make the socket listen to incoming connections
bool sock_listen(Sock *sock, int backlog);

// Accept connections from a socket
Sock *sock_accept(Sock *sock);

// Accept connections from a socket and handle them into a separate thread
bool sock_async_accept(Sock *sock, SockThreadFn fn, void *user_data);

// Connect a socket to a specific address
bool sock_connect(Sock *sock, SockAddr addr);

// Send data through a socket
ssize_t sock_send(Sock *sock, const void *buf, size_t size);

// Receive data from a socket
ssize_t sock_recv(Sock *sock, void *buf, size_t size);

// Send data through a socket in connectionless mode
ssize_t sock_sendto(Sock *sock, const void *buf, size_t size, SockAddr addr);

// Receive data from a socket in connectionless mode
ssize_t sock_recvfrom(Sock *sock, void *buf, size_t size, SockAddr *addr);

// Close a socket
void sock_close(Sock *sock);

// Log last error to stderr
void sock_log_error(const Sock *sock);

// Private functions
void *sock__accept_thread(void *data);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // SOCK_H_

#ifdef SOCK_IMPLEMENTATION

#ifdef __cplusplus
extern "C" { // Prevent name mangling
#endif // __cplusplus

Sock *sock_create(SockAddrType domain, SockType type)
{
    Sock *sock = (Sock*)malloc(sizeof(*sock));
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

    int enable = 1;
    if (setsockopt(sock->fd, SOL_SOCKET, SO_REUSEADDR,
                   &enable, sizeof(enable)) < 0) {
        free(sock);
        return NULL;
    }

    return sock;
}

SockAddr sock_addr(const char *addr, int port)
{
    SockAddr sa;
    memset(&sa, 0, sizeof(sa));

    if (addr == NULL) {
        return sa;
    }

    if (inet_pton(AF_INET, addr, &sa.ipv4.sin_addr) == 1) {
        sa.type = SOCK_IPV4;
        sa.ipv4.sin_family = AF_INET;
        sa.ipv4.sin_port = htons(port);
        sa.len = sizeof(sa.ipv4);
        strncpy(sa.str, addr, sizeof(sa.str));
    } else if (inet_pton(AF_INET6, addr, &sa.ipv6.sin6_addr) == 1) {
        sa.type = SOCK_IPV6;
        sa.ipv6.sin6_family = AF_INET6;
        sa.ipv6.sin6_port = htons(port);
        sa.len = sizeof(sa.ipv6);
        strncpy(sa.str, addr, sizeof(sa.str));
    } else {
        struct addrinfo *res;
        if (getaddrinfo(addr, NULL, NULL, &res) != 0) {
            // TODO: specific errors from getaddrinfo are ignored for logging
            sa.type = SOCK_ADDR_INVALID;
            return sa;
        }

        if (res->ai_family == AF_INET) {
            sa.type = SOCK_IPV4;
            sa.ipv4 = *(struct sockaddr_in*)res->ai_addr;
            sa.ipv4.sin_port = htons(port);
            sa.len = sizeof(sa.ipv4);
            inet_ntop(AF_INET, &sa.ipv4.sin_addr, sa.str, sizeof(sa.str));
        } else {
            sa.type = SOCK_IPV6;
            sa.ipv6 = *(struct sockaddr_in6*)res->ai_addr;
            sa.ipv6.sin6_port = htons(port);
            sa.len = sizeof(sa.ipv6);
            inet_ntop(AF_INET6, &sa.ipv6.sin6_addr, sa.str, sizeof(sa.str));
        }

        freeaddrinfo(res);
    }

    sa.port = port;

    return sa;
}

bool sock_bind(Sock *sock, SockAddr addr)
{
    if (sock == NULL) {
        return false;
    }

    if (bind(sock->fd, &addr.sockaddr, addr.len) < 0) {
        sock->last_error = strerror(errno);
        return false;
    }

    sock->addr = addr;

    return true;
}

bool sock_listen(Sock *sock, int backlog)
{
    if (sock == NULL) {
        return false;
    }

    if (listen(sock->fd, backlog) < 0) {
        sock->last_error = strerror(errno);
        return false;
    }

    return true;
}

Sock *sock_accept(Sock *sock)
{
    if (sock == NULL) {
        return NULL;
    }

    int fd = accept(sock->fd, &sock->addr.sockaddr, &sock->addr.len);
    if (fd < 0) {
        sock->last_error = strerror(errno);
        return NULL;
    }

    Sock *res = (Sock*)malloc(sizeof(*res));
    if (res == NULL) {
        sock->last_error = strerror(errno);
        return NULL;
    }
    memset(res, 0, sizeof(*res));

    res->type = sock->type;
    res->fd = fd;

    SockAddr *addr = &res->addr;
    addr->len = sizeof(addr->sockaddr);
    getpeername(res->fd, &addr->sockaddr, &addr->len);
    // TODO: This is duplicated code from sock_recvfrom
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

bool sock_async_accept(Sock *sock, SockThreadFn fn, void *user_data)
{
    if (sock == NULL) {
        return false;
    }

    Sock *client = sock_accept(sock);
    if (client == NULL) {
        return false;
    }

    SockThreadData *thread_data =
        (SockThreadData*)malloc(sizeof(*thread_data));
    if (thread_data == NULL) {
        sock_close(client);
        sock->last_error = strerror(errno);
        return false;
    }

    thread_data->fn = fn;
    thread_data->sock = client;
    thread_data->user_data = user_data;

    pthread_t thread;
    if (pthread_create(&thread, NULL, sock__accept_thread, thread_data) != 0) {
        free(thread_data);
        sock_close(client);
        sock->last_error = strerror(errno);
        return false;
    }

    pthread_detach(thread);

    return true;
}

bool sock_connect(Sock *sock, SockAddr addr)
{
    if (sock == NULL) {
        return false;
    }

    if (connect(sock->fd, &addr.sockaddr, addr.len) < 0) {
        return false;
    }

    sock->addr = addr;

    return true;
}

ssize_t sock_send(Sock *sock, const void *buf, size_t size)
{
    if (sock == NULL || buf == NULL) {
        return -1;
    }

    return send(sock->fd, buf, size, 0);
}

ssize_t sock_recv(Sock *sock, void *buf, size_t size)
{
    if (sock == NULL || buf == NULL) {
        return -1;
    }

    return recv(sock->fd, buf, size, 0);
}

ssize_t sock_sendto(Sock *sock, const void *buf, size_t size, SockAddr addr)
{
    if (sock == NULL || buf == NULL) {
        return -1;
    }

    return sendto(sock->fd, buf, size, 0, &addr.sockaddr, addr.len);
}

ssize_t sock_recvfrom(Sock *sock, void *buf, size_t size, SockAddr *addr)
{
    if (sock == NULL || buf == NULL) {
        return -1;
    }

    struct sockaddr *sa = NULL;
    socklen_t *len = NULL;

    if (addr != NULL) {
        sa = &addr->sockaddr;
        len = &sock->addr.len;
    }

    ssize_t res = recvfrom(sock->fd, buf, size, 0, sa, len);
    if (res < 0 || addr == NULL) {
        return res;
    }

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
    if (sock == NULL) {
        return;
    }

    close(sock->fd);
    free(sock);
}

void sock_log_error(const Sock *sock)
{
    if (sock == NULL) {
        fprintf(stderr, "SOCK ERROR: socket is NULL. errno says: %s\n",
                strerror(errno));
        return;
    }

    fprintf(stderr, "SOCK ERROR: %s\n", sock->last_error);
}

void *sock__accept_thread(void *data)
{
    SockThreadData *thread_data = (SockThreadData*) data;

    SockThreadFn fn = thread_data->fn;
    Sock *sock = thread_data->sock;
    void *user_data = thread_data->user_data;

    free(thread_data);

    fn(sock, user_data);

    return NULL;
}


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // SOCK_IMPLEMENTATION

/*
    Revision history:

        1.6.1 (2025-08-03) Log errno error if sock is NULL in sock_log_error
        1.6.0 (2025-08-03) Improve error logging; check for NULL pointers;
                           rename sock() -> sock_create()
        1.5.1 (2025-08-03) Improve support for C++
        1.5.0 (2025-05-20) Added new utility function `sock_async_accept` for
                           directly accepting new clients separate threads
        1.4.4 (2025-04-27) Fix NULL check of addr when calling recvfrom
        1.4.3 (2025-04-27) Enable socket option SO_REUSEADDR in sock()
        1.4.2 (2025-04-26) #include <stdio.h>
        1.4.1 (2025-04-26) Check if addr is NULL in sock_recvfrom
        1.4.0 (2025-04-26) Renamed sock_create() to sock()
        1.3.0 (2025-04-26) Renamed sock_log_errors to sock_log_error
        1.2.0 (2025-04-26) sock_addr can now resolve hostnames
        1.1.0 (2025-04-26) New sock_log_errors function
                           Fill new Sock's SockAddr after a sock_accept
                           Save remove SockAddr in Sock in sock_connect
        1.0.3 (2025-04-25) Fix incorrect usage of inet_pton
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
