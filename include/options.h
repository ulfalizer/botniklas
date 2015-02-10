// Channel to join after registering with the server.
extern const char *channel;
extern char       cmd_char;
extern const char *nick;
extern const char *port;
extern const char *quit_message;
extern const char *realname;
extern const char *server;
extern const char *username;

// If true, a trace of all messages received from the server is printed to
// stdout.
extern bool exit_on_invalid_msg;
extern bool trace_msgs;

void process_cmdline(int argc, char *argv[]);
