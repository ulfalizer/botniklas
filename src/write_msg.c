#include "common.h"
#include "dynamic_string.h"
#include "msg_io.h"
#include "options.h"

// Protects the write buffer and prevents the contents of multiple write()s
// from being interspersed (though the latter is unlikely to be a problem for
// the smallish writes we do).
static pthread_mutex_t write_mtx = PTHREAD_MUTEX_INITIALIZER;

static void lock_write() {
    int res;

    res = pthread_mutex_lock(&write_mtx);
    if (res != 0)
        err_n(res, "pthread_mutex_lock (write)");
}

static void unlock_write() {
    int res;

    res = pthread_mutex_unlock(&write_mtx);
    if (res != 0)
        err_n(res, "pthread_mutex_unlock (write)");
}

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
    int res;

    // TODO: This might need some adjustment once this function is used.

    string_free(&msg_write_buf);

    res = pthread_mutex_destroy(&write_mtx);
    if (res != 0)
        err_n(res, "pthread_mutex_destroy (write)");
}

void write_msg(int fd, const char *format, ...) {
    va_list ap;

    va_start(ap, format);

    lock_write();

    string_set_v(&msg_write_buf, format, ap);
    string_append(&msg_write_buf, "\r\n");
    writen(fd, string_get(&msg_write_buf), string_len(&msg_write_buf));

    unlock_write();

    va_end(ap);
}

void begin_msg() {
    lock_write();

    string_clear(&msg_write_buf);
}

void append_msg(const char *format, ...) {
    va_list ap;

    va_start(ap, format);
    string_append_v(&msg_write_buf, format, ap);
    va_end(ap);
}

void send_msg(int fd) {
    string_append(&msg_write_buf, "\r\n");
    writen(fd, string_get(&msg_write_buf), string_len(&msg_write_buf));

    unlock_write();
}
