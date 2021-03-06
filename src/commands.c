// Command implementations.

#include "common.h"
#include "commands.h"
#include "msg_io.h"
#include "options.h"
#include "remind.h"

static void compliment(const char *from, const char *to, const char *rep,
                       const char *arg) {
    say(rep, "You rock!");
}

static void echo(const char *from, const char *to, const char *rep,
                 const char *arg) {
    if (arg != NULL)
        say(rep, "%s", arg);
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
                 "Usage: !remind hh:mm[:ss] [dd/MM [yy]] <text of reminder>. "
                 "'yy' is nr. of years past 2000. Example: "
                 "!remind 14:45 11/2 do your laundry foobar, you slob.") };

static void commands(const char *from, const char *to, const char *rep,
                     const char *arg) {
    begin_say(rep);
    append_msg("Available commands:");
    for (size_t i = 0; i < ARRAY_LEN(cmds); ++i)
        append_msg(" !%s", cmds[i].cmd);
    send_msg();
}

static void help(const char *from, const char *to, const char *rep,
                 const char *arg) {
    if (arg == NULL) {
        say(rep, "Usage: !help <command>. Use !commands to list commands.");

        return;
    }

    for (size_t i = 0; i < ARRAY_LEN(cmds); ++i)
        if (strcmp(arg, cmds[i].cmd) == 0) {
            say(rep, "%s", cmds[i].help);

            return;
        }

    say(rep, "'%s': No such command. Use !commands to list commands.", arg);
}

void handle_cmd(const char *from, const char *to, const char *rep,
                const char *cmd, const char *arg) {
    for (size_t i = 0; i < ARRAY_LEN(cmds); ++i)
        if (strcmp(cmds[i].cmd, cmd) == 0) {
            cmds[i].handler(from, to, rep, arg);

            break;
        }
}
