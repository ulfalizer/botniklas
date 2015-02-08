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
    warning("Received ERROR message: %s", msg->params[0]);
}

static void handle_join(IRC_msg *msg) {
    log_join(msg->nick, msg->user, msg->host, msg->params[0]);
}

static void handle_kick(IRC_msg *msg) {
    log_kick(msg->nick, msg->params[0], msg->params[1],
             msg->n_params == 2 ? NULL : msg->params[2]);
}

static void handle_nick(IRC_msg *msg) {
    log_nick(msg->nick, msg->params[0]);
}

static void handle_part(IRC_msg *msg) {
    log_part(msg->nick, msg->user, msg->host, msg->params[0],
             msg->n_params == 1 ? NULL : msg->params[1]);
}

static void handle_ping(IRC_msg *msg) {
    write_msg("PONG :%s", msg->params[0]);
}

static void handle_privmsg(IRC_msg *msg) {
    log_privmsg(msg->nick, msg->params[0], msg->params[1]);

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
    log_quit(msg->nick, msg->user, msg->host,
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
    // Minimum number of parameters we expect in a valid message.
    size_t n_params_min;
    // Maximum number of parameters we expect in a valid message.
    size_t n_params_max;
    // true if we expect the message to include the sender's nickname in the
    // prefix.
    bool needs_nick;
} msgs[] = {
  { "001",     handle_welcome, 0, SIZE_MAX, false }, // RPL_WELCOME
  { "ERROR",   handle_error,   1, 1       , false },
  { "JOIN",    handle_join,    1, 1       , true  },
  { "KICK",    handle_kick,    2, 3       , true  },
  { "NICK",    handle_nick,    1, 1       , true  },
  { "PART",    handle_part,    1, 2       , true  },
  { "PING",    handle_ping,    1, 1       , false },
  { "PRIVMSG", handle_privmsg, 2, 2       , true  },
  { "QUIT",    handle_quit,    0, 1       , true  } };

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

            if (msgs[i].needs_nick && msg->nick == NULL) {
                warning("Ignoring %s lacking prefix with nickname.",
                        msg->cmd);

                if (exit_on_invalid_msg)
                    exit(EXIT_FAILURE);
            }

            msgs[i].handler(msg);

            break;
        }
}
