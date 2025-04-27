CC=gcc
CFLAGS=-Wall -Wextra -ggdb -I.
LDFLAGS=-lpthread

all: build/hello_server build/hello_client build/hello_server_v6 \
	build/hello_client_v6 build/chat_server build/chat_client \
	build/udp_server build/udp_client build/http_request

build/hello_server: examples/hello_server.c
	@mkdir -p build
	$(CC) $(CFLAGS) -o build/hello_server examples/hello_server.c $(LDFLAGS)

build/hello_client: examples/hello_client.c
	@mkdir -p build
	$(CC) $(CFLAGS) -o build/hello_client examples/hello_client.c $(LDFLAGS)

build/hello_server_v6: examples/hello_server_v6.c
	@mkdir -p build
	$(CC) $(CFLAGS) -o build/hello_server_v6 examples/hello_server_v6.c $(LDFLAGS)

build/hello_client_v6: examples/hello_client_v6.c
	@mkdir -p build
	$(CC) $(CFLAGS) -o build/hello_client_v6 examples/hello_client_v6.c $(LDFLAGS)

build/chat_server: examples/chat_server.c
	@mkdir -p build
	$(CC) $(CFLAGS) -o build/chat_server examples/chat_server.c $(LDFLAGS)

build/chat_client: examples/chat_client.c
	@mkdir -p build
	$(CC) $(CFLAGS) -o build/chat_client examples/chat_client.c $(LDFLAGS)

build/udp_server: examples/udp_server.c
	@mkdir -p build
	$(CC) $(CFLAGS) -o build/udp_server examples/udp_server.c $(LDFLAGS)

build/udp_client: examples/udp_client.c
	@mkdir -p build
	$(CC) $(CFLAGS) -o build/udp_client examples/udp_client.c $(LDFLAGS)

build/http_request: examples/http_request.c
	@mkdir -p build
	$(CC) $(CFLAGS) -o build/http_request examples/http_request.c $(LDFLAGS)

clean:
	rm -rf build
