#include "common.h"

#include <assert.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "fail.h"


int verbosity = 0;

static const unsigned int PORT = 7390;
static const char* const PORT_STR = "7390";


struct args {
    int dummy;
};


static void exit_with_usage(void)
{
    print(
        "Usage:  prog [OPTIONS]\n"
        "\n"
        "Options:\n"
        "  -h  Show this help message\n"
        "  -v  Be more verbose\n"
    );
    exit(E_USAGE);
}


static struct args get_args(int argc, char** argv)
{
    struct args args = {
        .dummy = 0,
    };

    bool help = false;

    opterr = 0; // Let me do my own error handling.
    while (true) {
        int c = getopt(argc, argv, "hv");
        if (c == -1)
            break;
        else if (c == '?')
            fatal(E_USAGE, "Unrecognized option '%c'", optopt);
        else if (c == 'h')
            help = true;
        else if (c == 'v')
            ++verbosity;
    }

    if (help)
        exit_with_usage();

    return args;
}


struct sockaddr_in6 get_addr(void) {
    const struct addrinfo hints = {
        .ai_flags = AI_ADDRCONFIG | AI_V4MAPPED | AI_NUMERICSERV,
        .ai_family = AF_INET6,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = 0,
    };
    struct addrinfo *res;
    int r = getaddrinfo("localhost", PORT_STR, &hints, &res);
    if (r != 0) {
        fatal(
            E_RARE,
            "can't determine server address (%s)",
            gai_strerror(r)
        );
    }
    assert(res != NULL);
    struct sockaddr_in6 addr = *(struct sockaddr_in6*)res->ai_addr;
    freeaddrinfo(res);
    return addr;
}


int connect_to_server(struct sockaddr_in6* const addr) {
    int sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (sock == -1) {
        fatal_e(E_RARE, "can't create socket");
    }
    int r = connect(sock, (struct sockaddr*)addr, sizeof(*addr));
    if (r == -1) {
        close(sock); // ignore errors
        fatal_e(E_COMMON, "can't connect to server");
    }
    static const size_t HOSTLEN = 1025;
    char* host = malloc(HOSTLEN);
    assert(host != NULL);
    r = getnameinfo(
        (struct sockaddr*)addr, sizeof(*addr),
        host, HOSTLEN,
        NULL, 0,
        NI_NUMERICHOST | NI_NUMERICSERV
    );
    if (r != 0) {
        fatal(
            E_RARE,
            "can't determine server address (%s)",
            gai_strerror(r)
        );
    }
    v1("Connected to host %s on port %s", host, PORT_STR);
    free(host);
    return sock;
}


int main(int argc, char** argv)
{
    struct args args = get_args(argc, argv);
    (void)args;

    struct sockaddr_in6 client_addr = get_addr();

    int sock = connect_to_server(&client_addr);
    close(sock); // ignore errors
}
