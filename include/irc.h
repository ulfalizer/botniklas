// Handle to the server's socket.
extern int serv_fd;

// Maximum number of parameters in IRC messages.
#define MAX_PARAMS 20

typedef struct IRC_msg {
    char *prefix; // Set to null if no prefix exists.
    char *cmd;
    char *params[MAX_PARAMS];
    size_t n_params;
} IRC_msg;

// Connects to the IRC server at 'host'/'port' and sends registration commands.
// Initializes 'serv_fd' on success. Exits the program if we fail to connect.
void connect_to_irc_server(const char *host, const char *port, const char *nick,
                           const char *username, const char *realname);

// Reads as much data as currently possible from the server and processes each
// received complete message. Data forming a partial message at the end is left
// in the read buffer for later.
//
// Intended to be called when we know that there is data, a connection error,
// or that the server closed the connection, so that we do not block.
//
// Returns false in case of an orderly shutdown from the server or a receive
// error.
bool process_msgs(void);

// Converts error replies (400-599) to their symbolic constants
// (401 -> "ERR_NOSUCHNICK", etc.).
const char *irc_errnum_str(unsigned errnum);
