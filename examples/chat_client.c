#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>

#define SOCK_IMPLEMENTATION
#include "sock.h"

#define DEFAULT_ADDRESS "127.0.0.1"
#define PORT 6969
#define BUFFER_CAPACITY 4096

bool disconnected = false;

void *listen_thread(void *user_data)
{
    Sock *server = (Sock*) user_data;

    char buffer[BUFFER_CAPACITY];
    ssize_t received = 0;

    while ((received = sock_recv(server, buffer, sizeof(buffer))) > 0) {
        printf("%.*s\n", (int)received, buffer);
    }

    disconnected = true;
    return NULL;
}

int main(int argc, char **argv)
{
    const char *address = DEFAULT_ADDRESS;
    if (argc > 1) {
        address = argv[1];
    }

    Sock *client = sock_create(SOCK_IPV4, SOCK_TCP);
    if (client == NULL) {
        fprintf(stderr, "ERROR: Could not create socket\n");
        return EXIT_FAILURE;
    }

    if (!sock_connect(client, sock_addr(address, PORT))) {
        fprintf(stderr, "ERROR: Could not connect to server\n");
        sock_close(client);
        return EXIT_FAILURE;
    }

    pthread_t ltid;
    if (pthread_create(&ltid, NULL, &listen_thread, client) != 0) {
        fprintf(stderr, "ERROR: Could not create listening thread\n");
        sock_close(client);
        return EXIT_FAILURE;
    }
    pthread_detach(ltid);

    printf("INFO: Connected to the server\n");
    printf("INFO: Type `disconnect` to disconnect from the server\n");

    char buffer[BUFFER_CAPACITY];
    while (!disconnected) {
        memset(buffer, 0, sizeof(buffer));
        fgets(buffer, sizeof(buffer), stdin);
        if (strcmp(buffer, "disconnect\n") == 0) {
            break;
        }
        size_t len = strlen(buffer)-1;
        sock_send(client, buffer, len);
    }

    sock_close(client);
    printf("INFO: Closed socket\n");

    return EXIT_SUCCESS;
}
