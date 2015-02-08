#include "common.h"
#include "chat_log.h"
#include "commands.h"
#include "irc.h"
#include "msg_io.h"
#include "msgs.h"
#include "options.h"

static void print_params(IRC_msg *msg) {
    if (msg->n_params == 0)
        fputs("(No parameters.)", stderr);
    else {
        fputs("Parameters:", stderr);
        for (size_t i = 0; i < msg->n_params; ++i)
            fprintf(stderr, " '%s'", msg->params[i]);
    }
}

static void handle_error(IRC_msg *msg) {
    fputs("warning: Received ERROR message. ", stderr);
    print_params(msg);
    putc('\n', stderr);
}

static void handle_ping(IRC_msg *msg) {
    write_msg("PONG :%s", msg->params[0]);
}

static void handle_privmsg(IRC_msg *msg) {
    char *ex_mark;

    // Ignore PRIVMSGs where the prefix is missing or we can't extract a
    // nick from it.
    if (msg->prefix == NULL ||
        (ex_mark = strchr(msg->prefix, '!')) == NULL)
        return;

    // Truncate prefix to just nick.
    *ex_mark = '\0';

    log_privmsg(msg->prefix, msg->params[0], msg->params[1]);

    // Look for bot command.
    if (msg->params[1][0] == '!') {
        char *arg;

        // The argument, if any, starts after the first space. We also treat an
        // empty argument as "no argument" as there's probably no sensible use
        // case there.
        arg = strchr(msg->params[1], ' ');
        if (arg != NULL) {
            // null-terminate command.
            *arg++ = '\0';
            if (*arg == '\0')
                arg = NULL;
        }

        handle_cmd(msg->prefix, msg->params[0],
                   // The "natural" reply target, passed as a convenience. This
                   // is either a channel for messages to a channel or the
                   // sending nick for messages directly to the bot.
                   is_channel(msg->params[0]) ? msg->params[0] : msg->prefix,
                   msg->params[1] + 1, // Command.
                   arg);
    }
}

static void handle_welcome(IRC_msg *msg) {
    write_msg("JOIN %s", channel);
}

static bool check_for_error_reply(IRC_msg *msg) {
    if (isdigit(msg->cmd[0]) && isdigit(msg->cmd[1]) && isdigit(msg->cmd[2]) &&
        msg->cmd[3] == '\0') {

        unsigned val = 100*(msg->cmd[0] - '0') + 10*(msg->cmd[1] - '0') +
                       (msg->cmd[2] - '0');

        if (val >= 400 && val <= 599) {
            fprintf(stderr, "warning: Received error reply %u (%s). ",
                    val, irc_errnum_str(val));
            print_params(msg);
            putc('\n', stderr);

            return true;
        }
    }

    return false;
}

#define DONT_CARE UINT_MAX

static const struct {
    const char *cmd;
    void (*handler)(IRC_msg *msg);
    size_t n_params_expected;
} msgs[] = {
  { "001",     handle_welcome, DONT_CARE }, // RPL_WELCOME
  { "ERROR",   handle_error,   DONT_CARE },
  { "PING",    handle_ping,    1         },
  { "PRIVMSG", handle_privmsg, 2         } };

void handle_msg(IRC_msg *msg) {
    if (check_for_error_reply(msg))
        return;

    for (size_t i = 0; i < ARRAY_LEN(msgs); ++i)
        if (strcmp(msgs[i].cmd, msg->cmd) == 0) {
            if (msgs[i].n_params_expected != DONT_CARE &&
                msg->n_params != msgs[i].n_params_expected) {

                warning("ignoring %s with %zu parameters (expected %zu)",
                        msg->cmd, msg->n_params, msgs[i].n_params_expected);

                if (exit_on_invalid_msg)
                    exit(EXIT_FAILURE);

                break;
            }

            msgs[i].handler(msg);

            break;
        }
}
