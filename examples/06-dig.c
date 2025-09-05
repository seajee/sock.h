#define SOCK_IMPLEMENTATION
#include "sock.h"

int main(int argc, char **argv)
{
    const char *program_name = argv[0];

    if (argc <= 1) {
        fprintf(stderr, "USAGE: %s <address>\n", program_name);
        fprintf(stderr, "ERROR: No address was provided\n");
        return 1;
    }

    const char *arg_address = argv[1];

    SockAddrList addrs = sock_dns(arg_address, 0, 0, SOCK_TCP);

    for (size_t i = 0; i < addrs.count; ++i) {
        SockAddr it = addrs.items[i];
        printf("%s\n", it.str);
    }

    sock_addr_list_free(&addrs);

    return 0;
}
