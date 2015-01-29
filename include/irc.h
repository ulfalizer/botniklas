// Maximum IRC message length, including the terminating '\r' or '\n'.
#define MAX_MSG_LEN 4096
// Maximum number of parameters in IRC messages.
#define MAX_PARAMS 20

typedef struct IRC_msg {
    char *prefix; // Set to null if no prefix exists.
    char *cmd;
    char *params[MAX_PARAMS];
    size_t n_params;
} IRC_msg;

// Connects to the IRC server at 'host'/'port' and sends registration commands.
int connect_to_irc_server(const char *host, const char *port, const char *nick,
                          const char *username, const char *realname);

// Reads and processes the next IRC message, skipping over invalid messages for
// first. Exits the program if a message longer than MAX_MSG_LEN is found.
void process_next_msg(int serv_fd);

// Converts error replies (400-599) to their symbolic constants
// (401 -> "ERR_NOSUCHNICK", etc.).
const char *irc_errnum_str(unsigned errnum);
