// Handle commands (PRIVMSGs starting with '!').
//
//   from: The nick of the user who sent the command.
//
//   to:   The channel in which the command was sent, or the nick of the bot in
//         case of a message directly to the bot.
//
//   rep:  The "natural" reply target. Either the channel where the command was
//         sent or the nick of the user who sent a message directly to the bot.
//
//   cmd:  The command (text after '!' until the end or the next ' ').
//
//   arg:  The command's argument: text after the ' ' after the command. NULL
//         in case the argument is missing (no ' ' after command) or empty
//         (single ' ' after command).
void handle_cmd(const char *from, const char *to, const char *rep,
                const char *cmd, const char *arg);
