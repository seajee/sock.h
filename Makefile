CC=gcc
CFLAGS=-Wall -Wextra -ggdb -I.
LDFLAGS=-lpthread

EXAMPLES=$(wildcard examples/*.c)
BUILDS=$(patsubst examples/%.c, build/%, $(EXAMPLES))

.PHONY: all clean

all: $(BUILDS)

build/%: examples/%.c sock.h build
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

build:
	mkdir -p build

clean:
	rm -rf build
