#include "common.h"
#include "date.h"
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

    when = parse_date(&cur);
    if (when == -1) {
        say(rep, "Error: Malformed or invalid time or date.");

        return;
    }

    if (cur[0] != ' ') {
        say(rep, "Error: Expected a space and the message after the time.");

        return;
    }

    if (cur[1] == '\0') {
        say(rep, "Error: Empty reminder message.");

        return;
    }

    ++cur;

    now = time(NULL);
    if (now == -1) {
        warning_err("time() failed (add reminder)");
        say(rep, "Failed to add reminder due to an unexpected error.");

        return;
    }

    if (when < now) {
        say(rep, "Error: That's in the past.");

        return;
    }

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

    // Reply with confirmation.

    time_t diff = when - now;
    unsigned n_days = diff/(60*60*24);
    unsigned n_hours = diff/(60*60)%24;
    unsigned n_minutes = diff/60%60;
    unsigned n_seconds = diff%60;

    begin_say(rep);
    append_msg("I will remind you in approx. ");
    if (n_days != 0)
        append_msg("%u day%s, ", n_days, n_days == 1 ? "" : "s");
    if (n_hours != 0)
        append_msg("%u hour%s, ", n_hours, n_hours == 1 ? "" : "s");
    if (n_minutes != 0)
        append_msg("%u minute%s, ", n_minutes, n_minutes == 1 ? "" : "s");
    append_msg("%u second%s!", n_seconds, n_seconds == 1 ? "" : "s");
    send_msg();
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
