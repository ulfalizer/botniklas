#include "common.h"
#include "commands.h"
#include "options.h"
#include "msg_write_buf.h"

// Returns the target to which a reply should be sent. Private messages
// generate replies directly to the user.
//
// TODO: Investigate robustness.
static char *reply_target(char *src, char *target) {
    char *end;

    if (strcmp(target, nick) == 0) {
        // Private message. See if 'src' contains the nick of the sender. We
        // expect the nick to appear before the first '!', if any.

        end = strchr(src, '!');
        if (end == NULL)
            return target;

        *end = '\0';

        // Return nick.
        return src;
    }

    return target;
}

// Command implementations. If the command is followed by a space, 'arg'
// contains the text after that. Otherwise, 'arg' is NULL.

static void compliment(int serv_fd, char *arg, char *src, char *target) {
    msg_write(serv_fd, "PRIVMSG %s :you rock!", reply_target(src, target));
}

static void echo(int serv_fd, char *arg, char *src, char *target) {
    if (arg != NULL && *arg != '\0')
        msg_write(serv_fd, "PRIVMSG %s :%s", reply_target(src, target), arg);
}

static void commands(int serv_fd, char *arg, char *src, char *target);

#define CMD(cmd) { #cmd, cmd }

static const struct {
    const char *const cmd;
    void (*const handler)(int serv_fd, char *arg, char *src, char *target);
} cmds[] = { CMD(commands),
             CMD(compliment),
             CMD(echo) };

static void commands(int serv_fd, char *arg, char *src, char *target) {
    msg_begin();
    msg_append("PRIVMSG %s :available commands:", reply_target(src, target));
    for (size_t i = 0; i < ARRAY_LEN(cmds); ++i)
        msg_append(" !%s", cmds[i].cmd);
    msg_send(serv_fd);
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
