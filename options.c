#include "common.h"
#include "options.h"

#define CHANNEL_DEFAULT "#botniklas"
#define NICK_DEFAULT "botniklas"
#define PORT_DEFAULT "6667"
#define REALNAME_DEFAULT NICK_DEFAULT
#define USERNAME_DEFAULT NICK_DEFAULT

// Option definitions and default values.

const char *channel = CHANNEL_DEFAULT;
const char *nick = NICK_DEFAULT;
const char *port = PORT_DEFAULT;
const char *realname = REALNAME_DEFAULT;
const char *server;
const char *username = USERNAME_DEFAULT;

bool exit_on_invalid_msg = false;
bool trace_msgs = false;

static void print_usage(char *argv[], FILE *stream) {
    fprintf(stream,
            "usage: %s [<options>] <server>\n"
            "\n"
            "<server> is the IRC server to connect to.\n"
            "\n"
            "<options>:\n"
            "  -c <channel to join> (default: \""CHANNEL_DEFAULT"\")\n"
            "     The channel name might have to be quoted to avoid\n"
            "     interpretation of '#' as the start of a comment.\n"
            "  -e  Exit the process when an invalid message is\n"
            "      received. Debugging helper.\n"
            "  -h  Print this usage message to stdout and exit. Other\n"
            "      arguments are ignored.\n"
            "  -n <nick to use> (default: \""NICK_DEFAULT"\")\n"
            "  -p <IRC server port> (default: \""PORT_DEFAULT"\")\n"
            "     Service names from /etc/services are also supported.\n"
            "  -r <realname to use> (default: \""REALNAME_DEFAULT"\")\n"
            "  -u <username to use> (default: \""USERNAME_DEFAULT"\")\n"
            "  -t  Print a trace of messages received from the server to stdout.\n",
            argv[0] ? argv[0] : "bot");
}

void process_cmdline(int argc, char *argv[]) {
    int opt;

    // Print errors ourself.
    opterr = 0;

    while ((opt = getopt(argc, argv, ":c:ehn:p:r:tu:")) != -1)
        switch (opt) {
        case 'c': channel = optarg; break;
        case 'e': exit_on_invalid_msg = true; break;
        case 'h': print_usage(argv, stdout); exit(EXIT_SUCCESS);
        case 'n': nick = optarg; break;
        case 'p': port = optarg; break;
        case 'r': realname = optarg; break;
        case 't': trace_msgs = true; break;
        case 'u': username = optarg; break;

        case '?':
            fprintf(stderr, "unknown flag '-%c'.\n\n", optopt);
            print_usage(argv, stderr);
            exit(EXIT_FAILURE);

        case ':':
            fprintf(stderr, "missing argument to '-%c'.\n\n", optopt);
            print_usage(argv, stderr);
            exit(EXIT_FAILURE);
        }

    if (argc - optind != 1) {
        fputs("expected a single non-flag argument (the IRC server).\n\n",
              stderr);
        print_usage(argv, stderr);
        exit(EXIT_FAILURE);
    }

    server = argv[optind];
}
