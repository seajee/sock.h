
    /***************************************************************\
    #                                                               #
    #              @@@@@@                                           #
    #              @    @                                           #
    #              @@@@@@                                           #
    #              @    @           sock.h - v1.7.0                 #
    #              @    @             MIT License                   #
    #            @@% .@ @                                           #
    #         @@     @  @    https://github.com/seajee/sock.h       #
    #        @ \    .@@                                             #
    #         @ @@@#                                                #
    #                                                               #
    \***************************************************************/


// sock.h - A single header library for network sockets management.
//
// [License and changelog]
//
//     See end of file.
//
// [Single header library usage]
//
//     Including this header file as such does what you normally expect.
//
//     If you also need to include all of the function implementations you
//     need to #define SOCK_IMPLEMENTATION before including the header file
//     like this:
//
//         #define SOCK_IMPLEMENTATION
//         #include "sock.h"
//
// [Structure documentation]
//
//     Sock:         can be treated as a normal socket
//
//     SockAddr:     an address composed of an IP (v4 or v6) and a port
//
//         The fields you will commonly refer to in this structure are:
//
//         SockAddr addr;
//         addr.port; // The port of the SockAddr
//         addr.str;  // String representation of the address
//
//     SockAddrList: a dynamic array of SockAddr
//
//         A SockAddrList can be iterated like so:
//
//         SockAddrList list = ...;
//         for (size_t i = 0; i < list.count; ++i) {
//             SockAddr addr = list.items[i];
//             // Try to connect to addr or just inspect it
//         }
//
// [Function documentation]
//
// In this library all of the major functions are abstraction layers to common
// standard functions, so their usage remain the same. Structures and utility
// functions are put in place to simplify the usage of the standard C API for
// socket and address handling.
//
// Sock related functions:
//
//     Sock *sock_create(SockAddrType domain, SockType type)
//
// Allocates and initializes a Sock with the corresponding domain and type.
// When you're done using it you should close it with sock_close(). Returns
// NULL on error.
//
//     bool sock_bind(Sock *sock, SockAddr addr)
//
// Binds a sock to the specified address. Returns false on error.
//
//     bool sock_listen(Sock *sock)
//
// Makes a sock listen for incoming connections. Returns false on error.
//
//     Sock *sock_accept(Sock *sock)
//
// Accepts a new connection on a sock in a blocking way. For handling new
// connections asynchronously you can use sock_async_accept(). Returns NULL on
// error.
//
//     bool sock_async_accept(Sock *sock, SockThreadCallback fn, void *user_data)
//
// Same as sock_accept but creates a new pthread that will own the client
// sock. It will be the callback job to close the new sock. A function
// callback shall be passed with the following signature:
//     void callback(Sock *sock, void *user_data)
// Returns false on error.
//
//     bool sock_connect(Sock *sock, SockAddr addr)
//
// Connects a sock on a connection-mode sock (e.g. TCP). Returns false on
// error.
//
//     ssize_t sock_send(Sock *sock, const void *buf, size_t size)
//
// Same as send() but with socks: sends the content of buf to the specified
// sock. On success returns the number of bytes sent. On error a negative
// number shall be returned.
//
//     ssize_t sock_recv(Sock *sock, void *buf, size_t size)
//
// Same as recv() but with socks: receives a message from sock end writes it
// into the specified buffer. On sucess returns the number of bytes received.
// On error a negative number shall be returned.
//
//     ssize_t sock_sendto(Sock *sock, const void *buf, size_t size, SockAddr addr)
//
// Same as sendto() but with socks: the return value works the same way as
// sock_send(). The addr parameter specifies the address to send the message
// to.
//
//     ssize_t sock_recvfrom(Sock *sock, void *buf, size_t size, SockAddr *addr)
//
// Same as recvfrom() but with socks: the return value works the same way as
// sock_recv(). The addr parameter will be filled with the address information
// of the sender.
//
//     void sock_close(Sock *sock);
//
// Closes a sock and releases its memory.
//
//     void sock_log_error(const Sock *sock)
//
// Prints the last error message of the specified Sock in stderr. This
// functions uses errno to get the error message.
//
// SockAddr related functions:
//
//     SockAddr sock_addr(const char *addr, int port)
//
// Given a valid IP (v4 or v6) address as a string and a port returns a
// correctly initialized SockAddr structure. On invalid input, the resulting
// SockAddr type will be set to SOCK_ADDR_INVALID.
//
//     SockAddrList sock_dns(const char *addr,
//                  int port, SockAddrType addr_hint, SockType sock_hint)
//
// This functions queries information about the specified address (e.g.
// "example.com") and returns a list of possible valid SockAddr. The only
// required parameter is addr, while the rest are optional hints that can be
// set to 0. Hints can be provided to further filter the resulting
// SockAddrList. An empty list will be returned in case of no results or on
// errors. Freeing the list is required after using it with
// sock_addr_list_free().
//
//     void sock_addr_list_free(SockAddrList *list)
//
// Releases the memory of the specified SockAddrList.

#ifndef SOCK_H_
#define SOCK_H_

#include <arpa/inet.h>
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

#define SOCK_ADDR_LIST_INITIAL_CAPACITY 16

#ifdef __cplusplus
extern "C" { // Prevent name mangling
#endif // __cplusplus

typedef enum {
    SOCK_ADDR_INVALID = 0,
    SOCK_IPV4 = AF_INET,
    SOCK_IPV6 = AF_INET6
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

typedef struct {
    SockAddr *items; // Dynamic array of SockAddr
    size_t count;
    size_t capacity;
} SockAddrList;

typedef enum {
    SOCK_TYPE_INVALID = 0,
    SOCK_TCP = SOCK_STREAM,
    SOCK_UDP = SOCK_DGRAM
} SockType;

typedef struct {
    SockType type;    // Socket type
    SockAddr addr;    // Socket address
    int fd;           // File descriptor
    int last_errno;  // Last error about this socket
} Sock;

typedef void (*SockThreadCallback)(Sock *sock, void *user_data);

typedef struct {
    SockThreadCallback callback;
    Sock *sock;
    void *user_data;
} SockThreadData;

// Create a socket with the corresponding domain and type
Sock *sock_create(SockAddrType domain, SockType type);

// Create a SockAddr structure from primitives
SockAddr sock_addr(const char *addr, int port);

// Get all possible addresses from DNS with optional hints
SockAddrList sock_dns(const char *addr, int port, SockAddrType addr_hint, SockType sock_hint);

// Free a SockAddrList structure
void sock_addr_list_free(SockAddrList *list);

// Bind a socket to a specific address
bool sock_bind(Sock *sock, SockAddr addr);

// Make the socket listen to incoming connections
bool sock_listen(Sock *sock);

// Accept connections from a socket
Sock *sock_accept(Sock *sock);

// Accept connections from a socket and handle them into a separate thread
bool sock_async_accept(Sock *sock, SockThreadCallback fn, void *user_data);

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
void sock__convert_addr(SockAddr *addr);

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
    sock->fd = socket(domain, type, 0);
    if (sock->fd < 0) {
        free(sock);
        return NULL;
    }

    int enable = 1;
    if (setsockopt(sock->fd, SOL_SOCKET, SO_REUSEADDR,
                   &enable, sizeof(enable)) < 0) {
        close(sock->fd);
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

    sa.port = port;

    if (inet_pton(AF_INET, addr, &sa.ipv4.sin_addr) == 1) {
        sa.type = SOCK_IPV4;
        sa.ipv4.sin_family = AF_INET;
        sa.ipv4.sin_port = htons(port);
        sa.len = sizeof(sa.ipv4);
        snprintf(sa.str, sizeof(sa.str), "%s", addr);
    } else if (inet_pton(AF_INET6, addr, &sa.ipv6.sin6_addr) == 1) {
        sa.type = SOCK_IPV6;
        sa.ipv6.sin6_family = AF_INET6;
        sa.ipv6.sin6_port = htons(port);
        sa.len = sizeof(sa.ipv6);
        snprintf(sa.str, sizeof(sa.str), "%s", addr);
    }

    return sa;
}

SockAddrList sock_dns(const char *addr, int port, SockAddrType addr_hint, SockType sock_hint)
{
    SockAddrList list;
    memset(&list, 0, sizeof(list));

    if (addr == NULL) {
        return list;
    }

    list.items = (SockAddr*)malloc(
            sizeof(*list.items) * SOCK_ADDR_LIST_INITIAL_CAPACITY);
    if (list.items == NULL) {
        return list;
    }
    list.capacity = SOCK_ADDR_LIST_INITIAL_CAPACITY;

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = (addr_hint != SOCK_ADDR_INVALID ? addr_hint : AF_UNSPEC);
    hints.ai_socktype = sock_hint;

    char port_str[8];
    snprintf(port_str, sizeof(port_str), "%d", port);

    struct addrinfo *res;
    if (getaddrinfo(addr, port_str, &hints, &res) != 0) {
        return list;
    }

    for (struct addrinfo *ai = res; ai != NULL; ai = ai->ai_next) {
        SockAddr sa;
        memset(&sa, 0, sizeof(sa));

        bool skip = false;

        switch (ai->ai_family) {
            case AF_INET: {
                struct sockaddr_in *sin = (struct sockaddr_in*)ai->ai_addr;
                sa.type = SOCK_IPV4;
                sa.port = ntohs(sin->sin_port);
                sa.ipv4 = *sin;
                sa.len = sizeof(sa.ipv4);
                inet_ntop(AF_INET, &sa.ipv4.sin_addr, sa.str, sizeof(sa.str));
            } break;

            case AF_INET6: {
                struct sockaddr_in6 *sin6 = (struct sockaddr_in6*)ai->ai_addr;
                sa.type = SOCK_IPV6;
                sa.port = ntohs(sin6->sin6_port);
                sa.ipv6 = *sin6;
                sa.len = sizeof(sa.ipv6);
                inet_ntop(AF_INET6, &sa.ipv6.sin6_addr, sa.str, sizeof(sa.str));
            } break;

            // Unreachable address family, skip address
            default: {
                skip = true;
            } break;
        }

        if (skip) {
            continue;
        }

        if (list.count >= list.capacity) {
            list.capacity *= 2;
            SockAddr *new_items = (SockAddr*)realloc(
                    list.items, list.capacity * sizeof(*list.items));
            if (new_items == NULL) {
                sock_addr_list_free(&list);
                return list;
            }
            list.items = new_items;
        }
        list.items[list.count++] = sa;
    }

    freeaddrinfo(res);

    return list;
}

void sock_addr_list_free(SockAddrList *list)
{
    if (list == NULL) {
        return;
    }

    free(list->items);
    list->count = 0;
    list->capacity = 0;
}

bool sock_bind(Sock *sock, SockAddr addr)
{
    if (sock == NULL) {
        return false;
    }

    if (bind(sock->fd, &addr.sockaddr, addr.len) < 0) {
        sock->last_errno = errno;
        return false;
    }

    sock->addr = addr;

    return true;
}

bool sock_listen(Sock *sock)
{
    if (sock == NULL) {
        return false;
    }

    if (listen(sock->fd, SOMAXCONN) < 0) {
        sock->last_errno = errno;
        return false;
    }

    return true;
}

Sock *sock_accept(Sock *sock)
{
    if (sock == NULL) {
        return NULL;
    }

    Sock *res = (Sock*)malloc(sizeof(*res));
    if (res == NULL) {
        sock->last_errno = errno;
        return NULL;
    }
    memset(res, 0, sizeof(*res));

    res->addr.len = sock->addr.len;

    int fd = accept(sock->fd, &res->addr.sockaddr, &res->addr.len);
    if (fd < 0) {
        free(res);
        sock->last_errno = errno;
        return NULL;
    }

    res->type = sock->type;
    res->fd = fd;
    sock__convert_addr(&res->addr);

    return res;
}

bool sock_async_accept(Sock *sock, SockThreadCallback fn, void *user_data)
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
        sock->last_errno = errno;
        return false;
    }

    thread_data->callback = fn;
    thread_data->sock = client;
    thread_data->user_data = user_data;

    pthread_t thread;
    if (pthread_create(&thread, NULL, sock__accept_thread, thread_data) != 0) {
        free(thread_data);
        sock_close(client);
        sock->last_errno = errno;
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
        sock->last_errno = errno;
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

    struct sockaddr_storage sa_storage;
    struct sockaddr *sa = NULL;
    socklen_t sa_len = 0;
    socklen_t *len_ptr = NULL;

    if (addr != NULL) {
        sa = (struct sockaddr*)&sa_storage;
        sa_len = sizeof(sa_storage);
        len_ptr = &sa_len;
    }

    ssize_t res = recvfrom(sock->fd, buf, size, 0, sa, len_ptr);
    if (res < 0 || addr == NULL) {
        return res;
    }

    if (addr != NULL) {
        memset(addr, 0, sizeof(*addr));
        memcpy(&addr->sockaddr, sa, sa_len);
        addr->len = sa_len;
        sock__convert_addr(addr);
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

    fprintf(stderr, "SOCK ERROR: %s\n", strerror(sock->last_errno));
}

void *sock__accept_thread(void *data)
{
    if (data == NULL) {
        return NULL;
    }

    SockThreadData *thread_data = (SockThreadData*) data;

    SockThreadCallback callback = thread_data->callback;
    Sock *sock = thread_data->sock;
    void *user_data = thread_data->user_data;

    free(thread_data);

    callback(sock, user_data);

    return NULL;
}

void sock__convert_addr(SockAddr *addr)
{
    if (addr == NULL) {
        return;
    }

    sa_family_t family = addr->sockaddr.sa_family;
    switch (family) {
        case AF_INET: {
            struct sockaddr_in *ipv4 = &addr->ipv4;
            addr->type = SOCK_IPV4;
            addr->port = ntohs(ipv4->sin_port);
            addr->len = sizeof(*ipv4);
            inet_ntop(family, &ipv4->sin_addr, addr->str, sizeof(addr->str));
        } break;

        case AF_INET6: {
            struct sockaddr_in6 *ipv6 = &addr->ipv6;
            addr->type = SOCK_IPV6;
            addr->port = ntohs(ipv6->sin6_port);
            addr->len = sizeof(*ipv6);
            inet_ntop(family, &ipv6->sin6_addr, addr->str, sizeof(addr->str));
        } break;

        default: {
            addr->type = SOCK_ADDR_INVALID;
        } break;
    }
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // SOCK_IMPLEMENTATION

/*
    Revision history:

        1.7.0 (2025-09-06) Header now includes documentation; New sock_dns()
                           function; major improvements and refactoring
        1.6.3 (2025-08-24) Change SockThreadFn -> SockThreadCallback
        1.6.2 (2025-08-03) Log errno error if sock last error is NULL
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
