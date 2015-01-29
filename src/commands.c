#include "common.h"
#include "commands.h"
#include "msg_io.h"
#include "options.h"
#include "remind.h"

// Command implementations. If the command is followed by a space, 'arg'
// contains the text after that. Otherwise, 'arg' is NULL.

static void compliment(int serv_fd, UNUSED char *arg, char *src, char *target) {
    write_msg(serv_fd, "PRIVMSG %s :You rock!", reply_target(src, target));
}

static void echo(int serv_fd, char *arg, char *src, char *target) {
    if (arg != NULL && *arg != '\0')
        write_msg(serv_fd, "PRIVMSG %s :%s", reply_target(src, target), arg);
}

static void remind(int serv_fd, char *arg, char *src, char *target) {
    handle_remind(serv_fd, arg, reply_target(src, target));
}

static void commands(int serv_fd, char *arg, char *src, char *target);

#define CMD(cmd) { #cmd, cmd }

static const struct {
    const char *const cmd;
    void (*const handler)(int serv_fd, char *arg, char *src, char *target);
} cmds[] = { CMD(commands),
             CMD(compliment),
             CMD(echo),
             CMD(remind) };

static void commands(int serv_fd, UNUSED char *arg, char *src, char *target) {
    begin_msg();
    append_msg("PRIVMSG %s :Available commands:", reply_target(src, target));
    for (size_t i = 0; i < ARRAY_LEN(cmds); ++i)
        append_msg(" !%s", cmds[i].cmd);
    send_msg(serv_fd);
}

void handle_cmd(int serv_fd, char *cmd, char *src, char *target) {
    for (size_t i = 0; i < ARRAY_LEN(cmds); ++i) {
        size_t cmd_len = strlen(cmds[i].cmd);

        if (strncmp(cmds[i].cmd, cmd, cmd_len) == 0 &&
            // The command must be followed by ' ' or '\0'.
            (cmd[cmd_len] == ' ' || cmd[cmd_len] == '\0')) {

            cmds[i].handler(serv_fd,
                            // Argument.
                            cmd[cmd_len] == '\0' ? NULL : cmd + cmd_len + 1,
                            src,
                            target);
            break;
        }
    }
}
