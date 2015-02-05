// Handles !remind messages. Registers a new pending reminder and saves it to
// disk (so we can reload it when the bot is restarted) if everything looks
// okay.
void handle_remind(const char *arg, const char *reply_target);

// Loads saved reminders from disk.
void restore_remind_state(void);
