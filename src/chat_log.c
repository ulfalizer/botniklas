#include "common.h"
#include "chat_log.h"
#include "files.h"

#define CHAT_LOG_FILE "chat_log"

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

void log_privmsg(const char *from, const char *to, const char *text) {
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

    if (fprintf(log_file, "%s  %s  <%s> %s\n", time_str, to, from, text) < 0)
        warning_err(PREFIX"fprintf() failed");

    #undef PREFIX

    if (fclose(log_file) == EOF)
        warning_err("close() failed on chat log file "
                    "('"CHAT_LOG_FILE"')");
}
