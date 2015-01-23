#include "common.h"
#include "dynamic_string.h"
#include "msg_write_buf.h"

static String msg_write_buf;

void msg_write_buf_init() {
    string_init(&msg_write_buf);
}

void msg_write_buf_free() {
    string_free(&msg_write_buf);
}

void msg_write(int fd, const char *format, ...) {
    va_list ap;

    va_start(ap, format);
    string_set_v(&msg_write_buf, format, ap);
    string_append(&msg_write_buf, "\r\n");
    va_end(ap);

    writen(fd, string_get(&msg_write_buf), string_len(&msg_write_buf));
}

void msg_begin() {
    string_clear(&msg_write_buf);
}

void msg_append(const char *format, ...) {
    va_list ap;

    va_start(ap, format);
    string_append_v(&msg_write_buf, format, ap);
    va_end(ap);
}

void msg_send(int fd) {
    string_append(&msg_write_buf, "\r\n");

    writen(fd, string_get(&msg_write_buf), string_len(&msg_write_buf));
}
