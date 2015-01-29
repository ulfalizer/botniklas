#include "common.h"
#include "msg_io.h"
#include "options.h"
#include "time_event.h"

typedef struct Reminder {
    int serv_fd;
    char msg[];
} Reminder;

static void remind(void *data) {
    Reminder *reminder = (Reminder*)data;

    write_msg(reminder->serv_fd, "PRIVMSG %s :REMINDER: %s", channel, reminder->msg);
    free(reminder);
}

// Parses calendar time in the format "hh:mm:ss dd/MM yy", where yy is years
// after 2000. Updates 'arg' to point just after the time, provided it's valid.
//
// Returns (time_t)-1 if the time format is invalid.
//
// TODO: Relax format and see if we can safely use sscanf().
static time_t parse_time(char **arg) {
    struct tm when_tm;
    char *s = *arg;

    if(!(// "hh:mm:ss "
         isdigit(s[0]) && isdigit(s[1]) && s[2] == ':' &&
         isdigit(s[3]) && isdigit(s[4]) && s[5] == ':' &&
         isdigit(s[6]) && isdigit(s[7]) && s[8] == ' ' &&
         // "dd/MM "
         isdigit(s[9]) && isdigit(s[10]) && s[11] == '/' &&
         isdigit(s[12]) && isdigit(s[13]) && s[14] == ' ' &&
         // "yy"
         isdigit(s[15]) && isdigit(s[16])))
        return -1;

    *arg += strlen("hh:mm:ss dd/MM yy");

    when_tm.tm_hour = 10*(s[0] - '0') + (s[1] - '0');
    when_tm.tm_min  = 10*(s[3] - '0') + (s[4] - '0');
    when_tm.tm_sec  = 10*(s[6] - '0') + (s[7] - '0');
    when_tm.tm_mday = 10*(s[9] - '0') + (s[10] - '0');
    when_tm.tm_mon  = 10*(s[12] - '0') + (s[13] - '0') - 1;
    when_tm.tm_year = 100 + 10*(s[15] - '0') + (s[16] - '0');
    when_tm.tm_isdst = -1; // Use DST information from locale.

    // Do some crude sanity checking before passing the time on to mktime(). It
    // should manage anyway (as fields are defined to overflow into one
    // another), but just to be paranoid.
    if (!(when_tm.tm_hour <= 23 &&
          when_tm.tm_min <= 59 &&
          when_tm.tm_sec <= 59 && // Disallow leap seconds. :P
          when_tm.tm_mday >= 1 && when_tm.tm_mday <= 31 &&
          when_tm.tm_mon >= 0 && when_tm.tm_mon <= 11 &&
          // Don't allow times past year 2037 (around where a signed 32-bit
          // time_t overflows) for now.
          when_tm.tm_year <= 137))
        return -1;

    return mktime(&when_tm);
}

void handle_remind(int serv_fd, char *arg, char *reply_target) {
    time_t now;
    Reminder *reminder;
    size_t reminder_len;
    time_t trig_time;

    if (arg == NULL) {
        write_msg(serv_fd, "PRIVMSG %s :Error: No time given.", reply_target);

        return;
    }

    trig_time = parse_time(&arg);
    if (trig_time == -1) {
        write_msg(serv_fd, "PRIVMSG %s :Error: Time is too wonky. Be more "
                           "straightforward.", reply_target);

        return;
    }

    now = time(NULL);
    if (now == -1)
        err("time (remind command)");

    if (trig_time < now) {
        write_msg(serv_fd, "PRIVMSG %s :Error: That's in the past.",
                  reply_target);

        return;
    }

    if (*arg++ == '\0') {
        write_msg(serv_fd, "PRIVMSG %s :Error: Missing reminder message.",
                  reply_target);

        return;
    }

    // Allocate reminder data.
    reminder_len = strlen(arg) + 1;
    reminder = emalloc(sizeof(*reminder) + reminder_len, "reminder data");
    reminder->serv_fd = serv_fd;
    memcpy(reminder->msg, arg, reminder_len);

    add_time_event(trig_time, remind, reminder);
}
