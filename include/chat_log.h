// Appends a JOIN to the chat log.
void log_join(const char *from, const char *channel);

// Appends a KICK to the chat log.
void log_kick(const char *from, const char *channel, const char *kickee,
              const char *text);

// Appends a NICK to the chat log.
void log_nick(const char *from, const char *to);

// Appends a PART to the chat log.
void log_part(const char *from, const char *channel);

// Appends a PRIVMSG to the chat log.
void log_privmsg(const char *from, const char *to, const char *text);

// Appends a QUIT to the chat log.
void log_quit(const char *from, const char *text);
