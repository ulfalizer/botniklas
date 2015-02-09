// 1337 monitor. Congratulates whoever writes "1337" first during 13:37 each
// day in #code.se.

// Initializes the 1337 monitor. Must be called before the function below.
void init_leet_monitor(void);

// Examines a PRIVMSG for the 1337 monitor.
void leet_monitor_privmsg(const char *nick, const char *to, const char *text);
