#include "common.h"

noreturn static void common_fail(bool print_errno, int errno_val,
                                 const char *format, va_list ap) {
    vfprintf(stderr, format, ap);
    if (print_errno)
        fprintf(stderr, ": %s", strerror(errno_val));
    putc('\n', stderr);
    exit(EXIT_FAILURE);
}

void err_exit(const char *format, ...) {
    va_list ap;

    va_start(ap, format);
    common_fail(true, errno, format, ap);
}

void err_exit_n(int errno_val, const char *format, ...) {
    va_list ap;

    va_start(ap, format);
    common_fail(true, errno_val, format, ap);
}

void fail_exit(const char *format, ...) {
    va_list ap;

    va_start(ap, format);
    common_fail(false, 0, format, ap);
}

void warning(const char *format, ...) {
    va_list ap;

    va_start(ap, format);
    fputs("warning: ", stderr);
    vfprintf(stderr, format, ap);
    putc('\n', stderr);
    va_end(ap);
}

void warning_err(const char *format, ...) {
    va_list ap;

    va_start(ap, format);
    fputs("warning: ", stderr);
    vfprintf(stderr, format, ap);
    fprintf(stderr, ": %s\n", strerror(errno));
    va_end(ap);
}

void *emalloc(size_t size, const char *desc) {
    void *res = malloc(size);

    if (res == NULL)
        err_exit("malloc failed: %s", desc);

    return res;
}

void *emalloc_align(size_t size, size_t align, const char *desc) {
    void *res;

    if (posix_memalign(&res, align, size) != 0)
        err_exit("posix_memalign failed: %s", desc);

    return res;
}

void *erealloc(void *ptr, size_t size, const char *desc) {
    void *res = realloc(ptr, size);

    if (res == NULL)
        err_exit("realloc failed: %s", desc);

    return res;
}

char *estrdup(const char *s, const char *desc) {
    char *res = strdup(s);

    if (res == NULL)
        err_exit("strdup failed: %s", desc);

    return res;
}

unsigned long long ge_pow_2(unsigned long long n) {
    // The generic method from
    // https://graphics.stanford.edu/~seander/bithacks.html is around 10%
    // slower than this version on my Core i7-2600K for a tight loop with
    // repeated calls.
    //
    // A look-up table (return ge_pow_2_table[__builtin_clzl(n - 1)]) is about
    // the same speed as this version (with 1, 2, 3, 4, ... as the input
    // sequence).
    //
    // A version based on e.g. log2() is more than 10 times slower than this
    // version.
    return n < 2 ? n : 1ULL << (CHAR_BIT*sizeof n - __builtin_clzll(n - 1));
}
