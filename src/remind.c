#include "common.h"
#include "files.h"
#include "msg_io.h"
#include "options.h"
#include "remind.h"
#include "time_event.h"

// File to keep a persistent record of reminders in. (Future) reminders are
// restored from this file on startup. Includes past reminders too - we only
// ever append to it.
#define REMINDERS_FILE "reminders"

// Appends a reminder to the file.
static void save_reminder(time_t when, const char *target,
                          const char *message) {
    FILE *remind_file;

    #define PREFIX "Failed to save reminder in '"REMINDERS_FILE"': "

    remind_file = open_file_stdio(REMINDERS_FILE, APPEND);
    if (remind_file == NULL) {
        warning_err(PREFIX"open_file_stdio() failed");

        return;
    }

    if (fprintf(remind_file, "%lld %s %s\n", (long long)when, target,
                message) < 0)
        warning_err(PREFIX"fprintf() failed");

    #undef PREFIX

    if (fclose(remind_file) == EOF)
        warning_err("close() failed on saved reminders file "
                    "('"REMINDERS_FILE"')");
}

// Reminder data is stored in a char array with the format
// "<target of message (channel or nick)>\0<reminder message>\0".

// Returns the target from a packed target and reminder string.
static char *target(char *target_and_reminder) {
    return target_and_reminder;
}

// Returns the reminder message from a packed target and reminder string.
static char *reminder(char *target_and_reminder) {
    return target_and_reminder + strlen(target_and_reminder) + 1;
}

// Callback called at the time of the reminder.
static void remind(void *data) {
    char *target_and_reminder = (char*)data;

    say(target(target_and_reminder), "REMINDER: %s",
        reminder(target_and_reminder));
    free(target_and_reminder);
}

// Parses calendar time in the format "hh:mm:ss dd/MM yy", where yy is years
// after 2000. Updates 'arg' to point just after the time, provided it's valid.
//
// Returns (time_t)-1 if the time format is invalid.
//
// TODO: Relax format and see if we can safely use sscanf().
static time_t parse_time(const char **arg) {
    struct tm when_tm;
    const char *s = *arg;

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

    when_tm.tm_hour  = 10*(s[0] - '0') + (s[1] - '0');
    when_tm.tm_min   = 10*(s[3] - '0') + (s[4] - '0');
    when_tm.tm_sec   = 10*(s[6] - '0') + (s[7] - '0');
    when_tm.tm_mday  = 10*(s[9] - '0') + (s[10] - '0');
    when_tm.tm_mon   = 10*(s[12] - '0') + (s[13] - '0') - 1;
    when_tm.tm_year  = 100 + 10*(s[15] - '0') + (s[16] - '0');
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

void handle_remind(const char *arg, const char *rep) {
    const char *cur;
    time_t now;
    char *reminder_data;
    size_t target_len;
    size_t reminder_len;
    time_t when;

    if (arg == NULL) {
        say(rep, "Error: No time given.");

        return;
    }

    cur = arg;

    when = parse_time(&cur);
    if (when == -1) {
        say(rep, "Error: Time is too wonky. Be more straightforward.");

        return;
    }

    now = time(NULL);
    if (now == -1)
        err_exit("time (remind command)");

    if (when < now) {
        say(rep, "Error: That's in the past.");

        return;
    }

    if (cur[0] == '\0' || cur[1] == '\0') {
        say(rep, "Error: Missing or empty reminder message.");

        return;
    }

    ++cur;

    // Save the reminder to the reminders file.
    save_reminder(when, rep, cur);

    // Allocate and initialize data for callback.

    target_len = strlen(rep) + 1;
    reminder_len = strlen(cur) + 1;

    reminder_data = emalloc(target_len + reminder_len, "reminder data");
    memcpy(reminder_data, rep, target_len);
    memcpy(reminder_data + target_len, cur, reminder_len);

    // Register callback.

    add_time_event(when, remind, reminder_data);
}

void restore_remind_state(void) {
    char *cur; // Current parsing location.
    char *end; // End sentinel.
    char *file_buf;
    size_t file_len;
    time_t now;

    now = time(NULL);
    if (now == -1)
        err_exit("time (load reminders)");

    file_buf = get_file_contents(REMINDERS_FILE, &file_len);
    if (file_buf == NULL)
        return;

    cur = file_buf;
    end = file_buf + file_len; // End sentinel.

    for (size_t line_nr = 1;; ++line_nr) {
        char *reminder_data;
        char *reminder_str;
        char *target_str;
        long long when;
        char *when_str;

        // Count a file with just a newline as empty too, for ease of manual
        // editing.
        if (cur == end || (*cur == '\n' && cur + 1 == end)) {
            free(file_buf);

            return;
        }

        #define EXPECT(cond, err_msg)                                            \
          if (!(cond)) {                                                         \
              warning("Invalid reminder on line %zu in '"REMINDERS_FILE"': %s. " \
                      "Ignoring that reminder as well as the remaining saved "   \
                      "reminders.", line_nr, err_msg);                           \
              free(file_buf);                                                    \
                                                                                 \
              return;                                                            \
          }

        #define EXPECT_CHAR(c, err_msg) \
          EXPECT(cur != end && *cur == c, err_msg)

        // Parse timestamp.
        for (when_str = cur; cur != end && isdigit(*cur); ++cur);
        EXPECT(cur > when_str, "Missing or malformed timestamp");
        EXPECT_CHAR(' ', "Expected space after timestamp");
        *cur++ = '\0';
        errno = 0;
        when = strtoll(when_str, NULL, 10);
        EXPECT(!(when == LLONG_MAX && errno == ERANGE) &&
               (time_t)when == when, // Truncation check.
               "Timestamp too large");

        // Parse target.
        for (target_str = cur; cur != end && *cur != ' '; ++cur);
        EXPECT(cur > target_str, "Missing or malformed target");
        EXPECT_CHAR(' ', "Expected space after target");
        *cur++ = '\0';

        // The reminder message consists of the the remaining characters before
        // the end of the line.
        for (reminder_str = cur; cur != end && *cur != '\n'; ++cur);
        EXPECT(cur > reminder_str, "Empty reminder message");
        EXPECT(cur != end, "Missing newline after reminder message");
        *cur++ = '\0';

        if (when < now)
            // Skip reminders from the past.
            continue;

        // Allocate and initialize reminder data. The target and reminder
        // strings already appear together in the correct format.

        reminder_data = emalloc(cur - target_str, "reminder data (from file)");
        memcpy(reminder_data, target_str, cur - target_str);

        // Register callback.

        add_time_event(when, remind, reminder_data);

        #undef EXPECT
        #undef EXPECT_CHAR
    }
}
