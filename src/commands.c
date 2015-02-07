#include "common.h"
#include "commands.h"
#include "msg_io.h"
#include "options.h"
#include "remind.h"

// Command implementations. If the command is followed by a space, 'arg'
// contains the text after that. Otherwise, 'arg' is NULL.

// TODO: Add some PRIVMSG-specific helpers.

static void compliment(const char *from, const char *to, const char *rep,
                       const char *arg) {
    write_msg("PRIVMSG %s :You rock!", rep);
}

static void echo(const char *from, const char *to, const char *rep,
                 const char *arg) {
    if (arg != NULL)
        write_msg("PRIVMSG %s :%s", rep, arg);
}

static void remind(const char *from, const char *to, const char *rep,
                   const char *arg) {
    handle_remind(arg, rep);
}

static void commands(const char *from, const char *to, const char *rep,
                     const char *arg);
static void help(const char *from, const char *to, const char *rep,
                 const char *arg);

#define CMD(cmd, help) { #cmd, cmd, help }

static const struct {
    const char *cmd;
    void (*handler)(const char *from, const char *to, const char *rep,
                    const char *arg);
    const char *help;
} cmds[] = { CMD(commands,
                 "Lists available commands."),
             CMD(compliment,
                 "Writes a compliment."),
             CMD(echo,
                 "Usage: !echo <text>"),
             CMD(help,
                 "Usage: !help <command>"),
             CMD(remind,
                 "Usage: !remind hh:mm:ss dd/MM yy <text of reminder>. yy is "
                 "nr. of years past 2000.") };

static void commands(const char *from, const char *to, const char *rep,
                     const char *arg) {
    begin_msg();
    append_msg("PRIVMSG %s :Available commands:", rep);
    for (size_t i = 0; i < ARRAY_LEN(cmds); ++i)
        append_msg(" !%s", cmds[i].cmd);
    send_msg();
}

static void help(const char *from, const char *to, const char *rep,
                 const char *arg) {
    if (arg == NULL) {
        write_msg("PRIVMSG %s :Usage: !help <command>. Use !commands to list "
                  "commands.", rep);

        return;
    }

    for (size_t i = 0; i < ARRAY_LEN(cmds); ++i)
        if (strcmp(arg, cmds[i].cmd) == 0) {
            write_msg("PRIVMSG %s :%s", rep, cmds[i].help);

            return;
        }

    write_msg("PRIVMSG %s :'%s': No such command. Use !command to list "
              "commands.", rep, arg);
}

void handle_cmd(const char *from, const char *to, const char *rep,
                const char *cmd, const char *arg) {
    for (size_t i = 0; i < ARRAY_LEN(cmds); ++i)
        if (strcmp(cmds[i].cmd, cmd) == 0) {
            cmds[i].handler(from, to, rep, arg);

            break;
        }
}
