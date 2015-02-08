// Appends a JOIN to the chat log.
void log_join(const char *nick, const char *user, const char *host,
              const char *channel);

// Appends a KICK to the chat log.
void log_kick(const char *nick, const char *channel, const char *kickee,
              const char *text);

// Appends a NICK to the chat log.
void log_nick(const char *nick, const char *to);

// Appends a PART to the chat log.
void log_part(const char *nick, const char *user, const char *host,
              const char *channel, const char *text);

// Appends a PRIVMSG to the chat log.
void log_privmsg(const char *nick, const char *to, const char *text);

// Appends a QUIT to the chat log.
void log_quit(const char *nick, const char *user, const char *host,
              const char *text);
