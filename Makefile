CC=gcc
CFLAGS=-Wall -Wextra -ggdb -I.
LDFLAGS=-lpthread

all: hello_server hello_client chat_server chat_client udp_server udp_client

chat_server: examples/chat_server.c
	$(CC) $(CFLAGS) -o chat_server examples/chat_server.c $(LDFLAGS)

chat_client: examples/chat_client.c
	$(CC) $(CFLAGS) -o chat_client examples/chat_client.c $(LDFLAGS)

hello_server: examples/hello_server.c
	$(CC) $(CFLAGS) -o hello_server examples/hello_server.c $(LDFLAGS)

hello_client: examples/hello_client.c
	$(CC) $(CFLAGS) -o hello_client examples/hello_client.c $(LDFLAGS)

udp_server: examples/udp_server.c
	$(CC) $(CFLAGS) -o udp_server examples/udp_server.c $(LDFLAGS)

udp_client: examples/udp_client.c
	$(CC) $(CFLAGS) -o udp_client examples/udp_client.c $(LDFLAGS)

clean:
	rm -rf chat_server
	rm -rf chat_client
	rm -rf hello_server
	rm -rf hello_client
	rm -rf udp_server
	rm -rf udp_client
