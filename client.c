#include "common.h"

#include <assert.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "fail.h"


int verbosity = 0;

static const unsigned int PORT = 7390;
static const char PORT_STR[] = "7390";


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


// TODO: actually test this
// also idk if it's useful at all
size_t my_strncpy(char* dest, const char* src, size_t dest_cap) {
    const char* orig_dest = dest;
    const char* dest_end = dest + dest_cap;
    while (dest < dest_end) {
        *dest = *src;
        ++dest;
        if (*src == 0) {
            break;
        }
        ++src;
    }
    return dest - orig_dest;
}


ssize_t send_all(int sockfd, const char* buf, size_t len, int flags) {
    ssize_t count;
    ssize_t count_total = 0;
    const char* buf_end = buf + len;
    while (buf < buf_end) {
        count = send(sockfd, buf, buf_end - buf, flags);
        if (count == -1) {
            if (count_total > 0) {
                return count_total; // TODO: this might be unsafe in some cases
            } else {
                return -1;
            }
        }
        count_total += count;
        buf += count;
    }
    return count_total;
}


void send_cmd(int sock, char cmd, const char* arg, const char* action) {
    size_t arg_len = strlen(arg);
    ssize_t count = send(sock, &cmd, 1, MSG_MORE);
    if (count == -1) {
        fatal_e(E_COMMON, "can't %s", action);
    }
    count = send_all(sock, arg, arg_len, MSG_MORE);
    if (count < (ssize_t)arg_len) {
        fatal_e(E_COMMON, "can't %s", action);
    }
    char end = 0;
    count = write(sock, &end, 1);
    if (count == -1) {
        fatal_e(E_COMMON, "can't %s", action);
    }
}


void set_name(int sock, const char* name) {
    send_cmd(sock, 'N', name, "set name");
}


void set_tag(int sock, const char* tag) {
    send_cmd(sock, 'T', tag, "set tag");
}


void send_message(int sock, const char* msg) {
    send_cmd(sock, 'M', msg, "send message");
}


void ui_loop(int sock) {
    static const size_t INPUT_BUF_CAP = 1024;
    char* input_buf = malloc(INPUT_BUF_CAP);
    while (true) {
        const char* r = fgets(input_buf, INPUT_BUF_CAP, stdin);
        if (r == NULL) {
            if (feof(stdin)) { // probably ^D
                break;
            }
            fatal(E_COMMON, "can't read from stdin");
        }
        size_t input_buf_len = strlen(input_buf);
        if (input_buf[input_buf_len - 1] != '\n') {
            fatal_e(E_COMMON, "input line too long or unexpected NUL");
        }
        input_buf[input_buf_len - 1] = 0; // strip newline

        static const char NAME_CMD[] = "/name ";
        static const size_t NAME_CMD_LEN = sizeof(NAME_CMD) - 1;

        static const char TAG_CMD[] = "/tag ";
        static const size_t TAG_CMD_LEN = sizeof(TAG_CMD) - 1;

        if (strncmp(input_buf, NAME_CMD, NAME_CMD_LEN) == 0) {
            char* name = input_buf + NAME_CMD_LEN;
            set_name(sock, name);
        } else if (strncmp(input_buf, TAG_CMD, TAG_CMD_LEN) == 0) {
            char* tag = input_buf + TAG_CMD_LEN;
            set_tag(sock, tag);
        } else {
            send_message(sock, input_buf);
        }
    }
    free(input_buf);
}


int main(int argc, char** argv)
{
    struct args args = get_args(argc, argv);
    (void)args;

    struct sockaddr_in6 client_addr = get_addr();

    int sock = connect_to_server(&client_addr);
    ui_loop(sock);
    close(sock); // ignore errors
}
