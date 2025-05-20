#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define SOCK_IMPLEMENTATION
#include "sock.h"

#define PORT 6969
#define POOL_CAPACITY 16
#define BUFFER_CAPACITY 4096
#define USERNAME_CAPACITY 16

Sock *socket_pool[POOL_CAPACITY];
pthread_mutex_t pool_lock;

bool add_client(Sock *sock)
{
    bool added = false;
    pthread_mutex_lock(&pool_lock);
    for (size_t i = 0; i < POOL_CAPACITY; ++i) {
        if (socket_pool[i] == NULL) {
            socket_pool[i] = sock;
            added = true;
            break;
        }
    }
    pthread_mutex_unlock(&pool_lock);

    return added;
}

bool remove_client(Sock *sock) {
    bool removed = false;
    pthread_mutex_lock(&pool_lock);
    for (size_t i = 0; i < POOL_CAPACITY; ++i) {
        if (socket_pool[i] == sock) {
            socket_pool[i] = NULL;
            removed = true;
            break;
        }
    }
    pthread_mutex_unlock(&pool_lock);

    return removed;
}

void broadcast(const Sock *from, const char *msg, size_t msg_len)
{
    pthread_mutex_lock(&pool_lock);
    for (size_t i = 0; i < POOL_CAPACITY; ++i) {
        Sock *r = socket_pool[i];
        if (r != NULL && r != from) {
            sock_send(r, msg, msg_len);
        }
    }
    pthread_mutex_unlock(&pool_lock);
}

void handle_client(Sock *client, void *user_data)
{
    (void) user_data;

    if (!add_client(client)) {
        fprintf(stderr, "ERROR: TCP_Socket pool buffer is full (%d)\n",
                POOL_CAPACITY);
        sock_close(client);
        return;
    }

    printf("INFO: New client connected from %s:%d\n", client->addr.str,
            client->addr.port);

    ssize_t received = 0;
    char buffer[BUFFER_CAPACITY];
    memset(buffer, 0, sizeof(buffer));

    char username[USERNAME_CAPACITY];
    size_t username_length = 0;
    memset(username, 0, sizeof(username));

    const char *username_prompt = "username:";
    size_t prompt_length = strlen(username_prompt);
    memcpy(buffer, "username: ", prompt_length);
    sock_send(client, buffer, prompt_length);
    received = sock_recv(client, username, sizeof(username));
    if (received < 0) {
        goto disconnect;
    }
    username_length = received;

    printf("INFO: Client login with username `%.*s`\n", (int)username_length,
            username);
    received = snprintf(buffer, sizeof(buffer), "[Server] `%.*s` joined the chat",
                        (int)username_length, username);
    broadcast(client, buffer, received);

    memcpy(buffer, username, username_length);
    buffer[username_length] = ':';
    buffer[username_length + 1] = ' ';
    size_t prefix_len = username_length + 2;

    size_t read_len = sizeof(buffer) - prefix_len;
    char *read_buf = buffer + prefix_len;

    while ((received = sock_recv(client, read_buf, read_len)) > 0) {
        printf("INFO: %.*s: %.*s\n", (int)username_length, username,
                (int)received, read_buf);
        broadcast(client, buffer, received + prefix_len);
    }

disconnect:
    sock_close(client);
    remove_client(client);

    printf("INFO: Client `%.*s` disconnected\n", (int)username_length,
            username);
    received = snprintf(buffer, sizeof(buffer), "[Server] `%.*s` left the chat",
            (int)username_length, username);
    broadcast(client, buffer, received);
}

int main(void)
{
    Sock *server = sock(SOCK_IPV4, SOCK_TCP);
    if (server == NULL) {
        fprintf(stderr, "ERROR: Could not create socket\n");
        return EXIT_FAILURE;
    }

    printf("INFO: Created socket\n");

    if (!sock_bind(server, sock_addr("0.0.0.0", PORT))) {
        fprintf(stderr, "ERROR: Could not bind socket\n");
        sock_close(server);
        return EXIT_FAILURE;
    }

    printf("INFO: Bind socket\n");

    if (!sock_listen(server, 10)) {
        fprintf(stderr, "ERROR: Could not listen on socket\n");
        sock_close(server);
        return EXIT_FAILURE;
    }

    printf("INFO: Listen socket\n");

    while (true) {
        if (!sock_async_accept(server, handle_client, NULL)) {
            fprintf(stderr, "ERROR: Could not accept client\n");
            continue;
        }
    }

    sock_close(server);
    printf("INFO: Closed socket\n");

    return EXIT_SUCCESS;
}
