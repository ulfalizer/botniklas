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

// TODO: Later on we might want to parse out the nick and host and save them in
// IRC_msg instead.
static bool truncate_prefix_to_nick(IRC_msg *msg) {
    char *ex_mark;

    // Return an error if the prefix is missing or we can't extract a nick from
    // it.
    if (msg->prefix == NULL ||
        (ex_mark = strchr(msg->prefix, '!')) == NULL)
        return false;

    *ex_mark = '\0';

    return true;
}

static void handle_error(IRC_msg *msg) {
    warning("Received ERROR message: %s", msg->params[0]);
}

static void handle_join(IRC_msg *msg) {
    if (!truncate_prefix_to_nick(msg))
        return;

    log_join(msg->prefix, msg->params[0]);
}

static void handle_kick(IRC_msg *msg) {
    if (!truncate_prefix_to_nick(msg))
        return;

    log_kick(msg->prefix, msg->params[0], msg->params[1],
             msg->n_params == 2 ? NULL : msg->params[2]);
}

static void handle_nick(IRC_msg *msg) {
    if (!truncate_prefix_to_nick(msg))
        return;

    log_nick(msg->prefix, msg->params[0]);
}

static void handle_part(IRC_msg *msg) {
    if (!truncate_prefix_to_nick(msg))
        return;

    log_part(msg->prefix, msg->params[0]);
}

static void handle_ping(IRC_msg *msg) {
    write_msg("PONG :%s", msg->params[0]);
}

static void handle_privmsg(IRC_msg *msg) {
    if (!truncate_prefix_to_nick(msg))
        return;

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

static void handle_quit(IRC_msg *msg) {
    if (!truncate_prefix_to_nick(msg))
        return;

    log_quit(msg->prefix,
             msg->n_params == 0 ? NULL : msg->params[0]);
}

static void handle_welcome(IRC_msg *msg) {
    write_msg("JOIN %s", channel);
}

// Checks if 'msg' is an error reply (a numeric reply in the range 400-599) and
// prints it and returns true if so.
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

static const struct {
    const char *cmd;
    void (*handler)(IRC_msg *msg);
    size_t n_params_min;
    size_t n_params_max;
} msgs[] = {
  { "001",     handle_welcome, 0, SIZE_MAX }, // RPL_WELCOME
  { "ERROR",   handle_error,   1, 1        },
  { "JOIN",    handle_join,    1, 1        },
  { "KICK",    handle_kick,    2, 3        },
  { "NICK",    handle_nick,    1, 1        },
  { "PART",    handle_part,    1, 1        },
  { "PING",    handle_ping,    1, 1        },
  { "PRIVMSG", handle_privmsg, 2, 2        },
  { "QUIT",    handle_quit,    0, 1        } };

void handle_msg(IRC_msg *msg) {
    if (check_for_error_reply(msg))
        return;

    for (size_t i = 0; i < ARRAY_LEN(msgs); ++i)
        if (strcmp(msgs[i].cmd, msg->cmd) == 0) {
            if (msg->n_params < msgs[i].n_params_min ||
                msg->n_params > msgs[i].n_params_max) {

                warning("Ignoring %s with %zu parameters (expected between "
                        "%zu and %zu parameters)", msg->cmd, msg->n_params,
                        msgs[i].n_params_min, msgs[i].n_params_max);

                if (exit_on_invalid_msg)
                    exit(EXIT_FAILURE);

                break;
            }

            msgs[i].handler(msg);

            break;
        }
}
