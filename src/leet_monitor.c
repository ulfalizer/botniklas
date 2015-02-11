#include "common.h"
#include "date.h"
#include "leet_monitor.h"
#include "msg_io.h"
#include "time_event.h"

#define LEET_CHANNEL "#code.se"
// To be able to quickly change the trigger time during testing.
#define LEET_HOUR 13
#define LEET_MINUTE 37

// True if the current time is between 13:37 and 13:38 and no one has said
// "1337" yet.
static bool want_1337 = false;

// Called at 13:37.
static void at_1337(void *data) {
    want_1337 = true;
}

static void schedule_next_1337(void);

// Called at 13:38.
static void at_1338(void *data) {
    if (want_1337) {
        want_1337 = false;
        say(LEET_CHANNEL, "No one was 1337 today. :(");
    }
    schedule_next_1337();
}

// Sets up at_1337() and at_1338() to be called when those times next come
// around.
static void schedule_next_1337(void) {
    struct tm tm;

    if (!get_current_time(&tm, "leet monitor schedule"))
        return;

    if (tm.tm_hour > LEET_HOUR || (tm.tm_hour == LEET_HOUR &&
                                   tm.tm_min >= LEET_MINUTE))
        // We're already past 13:37:00 today. Schedule for tomorrow.
        ++tm.tm_mday;
    tm.tm_hour = LEET_HOUR;
    tm.tm_min = LEET_MINUTE;
    tm.tm_sec = 0;
    // Automatically deduce whether daylight saving time is in effect.
    tm.tm_isdst = -1;

    if (!add_time_event_tm(&tm, at_1337, NULL))
        // Keep going. It can't get any worse if we try to add at_1338().
        warning("Failed to add 13:37 time event for leet monitor");

    tm.tm_min = LEET_MINUTE + 1;
    tm.tm_isdst = -1; // In case it's different the next minute.
    if (!add_time_event_tm(&tm, at_1338, NULL))
        warning("Failed to add 13:38 time event for leet monitor");
}

void leet_monitor_privmsg(const char *nick, const char *to,
                          const char *text) {
    if (want_1337 && strcmp(to, LEET_CHANNEL) == 0 &&
        strstr(text, "1337") != NULL) {

        say(LEET_CHANNEL, "%s is the 1337est!!!", nick);
        want_1337 = false;
    }
}

void init_leet_monitor(void) {
    schedule_next_1337();
}
