#include "common.h"

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "fail.h"


int verbosity = 0;


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


int main(int argc, char** argv)
{
    struct args args = get_args(argc, argv);
    (void)args;

    v0("Verbosity is at least 0.");
    v1("Verbosity is at least 1.");
    v2("Verbosity is at least 2.");
    warning("This is a warning.");
    fatal(E_COMMON, "This is an error.");
}
