#include "common.h"
#include "dynamic_string.h"
#include "irc.h"
#include "msg_io.h"
#include "options.h"

static String msg_write_buf;

void msg_write_buf_init(void) {
    string_init(&msg_write_buf);
}

void msg_write_buf_free(void) {
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

void begin_msg(void) {
    string_clear(&msg_write_buf);
}

void append_msg(const char *format, ...) {
    va_list ap;

    va_start(ap, format);
    string_append_v(&msg_write_buf, format, ap);
    va_end(ap);
}

void send_msg(void) {
    string_append(&msg_write_buf, "\r\n");
    writen(serv_fd, string_get(&msg_write_buf), string_len(&msg_write_buf));
}

void say(const char *to, const char *format, ...) {
    va_list ap;

    va_start(ap, format);
    string_set(&msg_write_buf, "PRIVMSG %s :", to);
    string_append_v(&msg_write_buf, format, ap);
    string_append(&msg_write_buf, "\r\n");
    writen(serv_fd, string_get(&msg_write_buf), string_len(&msg_write_buf));
    va_end(ap);
}

void begin_say(const char *to) {
    string_set(&msg_write_buf, "PRIVMSG %s :", to);
}
