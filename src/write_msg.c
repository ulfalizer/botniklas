#include "common.h"
#include "dynamic_string.h"
#include "irc.h"
#include "msg_io.h"
#include "options.h"

// TODO: Investigate robustness.
char *reply_target(char *src, char *target) {
    char *end;

    if (strcmp(target, nick) == 0) {
        // Private message. See if 'src' contains the nick of the sender. We
        // expect the nick to appear before the first '!', if any.

        end = strchr(src, '!');
        if (end == NULL)
            return target;

        *end = '\0';

        // Return nick.
        return src;
    }

    return target;
}

static String msg_write_buf;

void msg_write_buf_init() {
    string_init(&msg_write_buf);
}

void msg_write_buf_free() {
    string_free(&msg_write_buf);
}

void write_msg(const char *format, ...) {
    va_list ap;

    va_start(ap, format);
    string_set_v(&msg_write_buf, format, ap);
    string_append(&msg_write_buf, "\r\n");
    writen(serv_fd, string_get(&msg_write_buf), string_len(&msg_write_buf));
    va_end(ap);
}

void begin_msg() {
    string_clear(&msg_write_buf);
}

void append_msg(const char *format, ...) {
    va_list ap;

    va_start(ap, format);
    string_append_v(&msg_write_buf, format, ap);
    va_end(ap);
}

void send_msg() {
    string_append(&msg_write_buf, "\r\n");
    writen(serv_fd, string_get(&msg_write_buf), string_len(&msg_write_buf));
}
