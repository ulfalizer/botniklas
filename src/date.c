#include "common.h"
#include "date.h"

bool get_current_time(struct tm *tm, const char *context) {
    time_t now;

    now = time(NULL);
    if (now == -1) {
        warning_err("time() failed (%s)", context);

        return false;
    }

    if (localtime_r(&now, tm) == NULL) {
        warning("localtime_r() failed (%s)", context);

        return false;
    }

    return true;
}

// Parses a one- or two-digit number from 's' and stores it in 'val', updating
// 's' to point one character past the final digit. Any amount of whitespace is
// accepted before the number if 'accept_leading_ws' is true.
//
// Returns 'false' without updating 's' if the first character (possibly
// preceded by whitespace) is not a digit.
static bool parse_one_or_two_digits(const char **s, int *val,
                                    bool accept_leading_ws) {
    const char *cur = *s;

    if (accept_leading_ws)
        while (isspace(*cur))
            ++cur;

    if (!isdigit(*cur))
        return false;
    *val = (*cur++ - '0');
    if (isdigit(*cur)) {
        *val = 10*(*val) + (*cur - '0');
        ++cur;
    }
    *s = cur;

    return true;
}

time_t parse_date(const char **s) {
    const char *cur = *s;
    time_t res;
    struct tm tm;
    struct tm tm_orig;

    // Components that are not specified are taken to be the current time
    // (except for seconds, which are taken to be 0).
    if (!get_current_time(&tm, "parse date"))
        return -1;
    tm.tm_sec = 0;

    // Expects and removes 'c' from the input.
    #define EAT(c)     \
      if (*cur++ != c) \
          return -1

    // Parse hour.
    if (!parse_one_or_two_digits(&cur, &tm.tm_hour, true))
        return -1;
    EAT(':');

    // Parse minute.
    if (!parse_one_or_two_digits(&cur, &tm.tm_min, false))
        return -1;
    switch (*cur) {
    case ':': ++cur; break;
    case ' ': goto parse_day;
    default: goto got_time;
    }

    // Parse second.
    if (!parse_one_or_two_digits(&cur, &tm.tm_sec, false))
        return -1;
    if (*cur != ' ')
        goto got_time;

parse_day:
    // Parse day.
    if (!parse_one_or_two_digits(&cur, &tm.tm_mday, true))
        goto got_time;
    EAT('/');

    // Parse month.
    if (!parse_one_or_two_digits(&cur, &tm.tm_mon, false))
        return -1;
    --tm.tm_mon;
    if (*cur != ' ')
        goto got_time;

    // Parse year.
    if (!parse_one_or_two_digits(&cur, &tm.tm_year, true))
        goto got_time;
    // Always disallow years past 2038 (Unix "Y2K") for now.
    if (tm.tm_year >= 38)
        return -1;
    tm.tm_year += 100;

    #undef EAT

got_time:
    tm.tm_isdst = -1; // Use DST information from locale.

    tm_orig = tm;
    res = mktime(&tm);
    if (res == -1) {
        warning("mktime() failed (parse date)");

        return -1;
    }

    // Assume the given time is invalid if any of the fields were adjusted by
    // mktime().
    if (tm.tm_hour != tm_orig.tm_hour ||
        tm.tm_min  != tm_orig.tm_min  ||
        tm.tm_sec  != tm_orig.tm_sec  ||
        tm.tm_mday != tm_orig.tm_mday ||
        tm.tm_mon  != tm_orig.tm_mon  ||
        tm.tm_year != tm_orig.tm_year)
        return -1;

    *s = cur;

    return res;
}
