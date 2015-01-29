// Handle commands (PRIVMSGs starting with '!').
//
//   cmd: The message's text, without the leading '!'.
//   src: The message's source (prefix).
//   target: The message's target.
void handle_cmd(int serv_fd, char *cmd, char *src, char *target);
