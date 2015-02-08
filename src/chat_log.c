#include "common.h"
#include "chat_log.h"
#include "files.h"

#define CHAT_LOG_FILE "chat_log"

static bool format_now(const char *format, char *buf, size_t buf_len)
    __attribute__((format(strftime, 1, 0)));

static bool format_now(const char *format, char *buf, size_t buf_len) {
    time_t now;
    struct tm now_tm;

    now = time(NULL);
    if (now == -1) {
        warning_err("time() failed (chat log)");

        return false;
    }

    if (localtime_r(&now, &now_tm) == NULL) {
        warning("localtime_r() failed (chat log)");

        return false;
    }

    if (strftime(buf, buf_len, format, &now_tm) == 0) {
        warning("strftime() failed (chat log)");

        return false;
    }

    return true;
}

static void log_append(const char *format, ...)
    __attribute__((format(printf, 1, 2)));

static void log_append(const char *format, ...) {
    va_list ap;
    FILE *log_file;
    char time_str[64];

    #define PREFIX "Failed to append chat log entry to '"CHAT_LOG_FILE"': "

    if (!format_now("%c", time_str, sizeof time_str)) {
        warning(PREFIX"Could not get current time");

        return;
    }

    log_file = open_file_stdio(CHAT_LOG_FILE, APPEND);
    if (log_file == NULL) {
        warning_err(PREFIX"open_file_stdio() failed");

        return;
    }

    if (fprintf(log_file, "%s  ", time_str) < 0) {
        warning_err(PREFIX"fprintf() failed");

        goto close_log;
    }

    va_start(ap, format);
    if (vfprintf(log_file, format, ap) < 0) {
        warning_err(PREFIX"vfprintf() failed");

        goto close_log;
    }

    if (fputc('\n', log_file) == EOF)
        warning_err(PREFIX"fputc() failed");

    #undef PREFIX

close_log:
    va_end(ap);
    if (fclose(log_file) == EOF)
        warning_err("fclose() failed on chat log file ('"CHAT_LOG_FILE"')");
}

void log_join(const char *nick, const char *user, const char *host,
              const char *channel) {
    log_append("%s  %s (%s@%s) joined", channel, nick, user,
               host ? host : "<unknown>");
}

void log_kick(const char *nick, const char *channel, const char *kickee,
              const char *text) {
    if (text == NULL)
        log_append("%s  %s was kicked by %s", channel, kickee, nick);
    else
        log_append("%s  %s was kicked by %s: %s", channel, kickee, nick, text);
}

void log_nick(const char *nick, const char *to) {
    log_append("%s changed nick to %s", nick, to);
}

void log_part(const char *nick, const char *user, const char *host,
              const char *channel, const char *text) {
    if (text == NULL)
        log_append("%s  %s (%s@%s) left", channel, nick, user,
                   host ? host : "<unknown>");
    else
        log_append("%s  %s (%s@%s) left: %s", channel, nick, user,
                   host ? host : "<unknown>", text);
}

void log_privmsg(const char *nick, const char *to, const char *text) {
    log_append("%s  <%s> %s", to, nick, text);
}

void log_quit(const char *nick, const char *user, const char *host,
              const char *text) {
    if (text == NULL)
        log_append("%s (%s@%s) quit", nick, user, host ? host : "<unknown>");
    else
        log_append("%s (%s@%s) quit: %s", nick, user,
                   host ? host : "<unknown>", text);
}
