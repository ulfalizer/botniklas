#include "common.h"

char *get_file_contents(const char *filename, size_t *len, bool *exists) {
    char *buf;
    int fd;
    struct stat fd_stat;
    ssize_t n_read;
    size_t n_read_tot;

    *exists = true;

    fd = open(filename, O_RDONLY | O_CLOEXEC);
    if (fd == -1) {
        if (errno == ENOENT)
            *exists = false;
        else
            warning("open() error on '%s': %s", filename, strerror(errno));

        return NULL;
    }

    if (fstat(fd, &fd_stat) == -1) {
        warning("fstat() error on '%s': %s", filename, strerror(errno));

        goto err_close;
    }

    if (S_ISDIR(fd_stat.st_mode)) {
        warning("error while opening '%s': Is a directory", filename);

        goto err_close;
    }

    if (!S_ISREG(fd_stat.st_mode)) {
        warning("error while opening '%s': Not a regular file", filename);

        goto err_close;
    }

    buf = emalloc(fd_stat.st_size, "file buffer");

    // The file size could change between the fstat() and read(), so read up to
    // the minimum of the buffer size and end of file. It's unlikely that a
    // read() on a regular file would be interruptible, but play it safe.

    for (n_read_tot = 0; n_read_tot < fd_stat.st_size; n_read_tot += n_read) {
again:
        n_read = read(fd, buf + n_read_tot, fd_stat.st_size - n_read_tot);

        if (n_read == 0)
            // EOF.
            break;

        if (n_read == -1) {
            if (errno == EINTR)
                goto again;

            warning("read() error on '%s': %s", filename, strerror(errno));

            goto err_free_close;
        }
    }

    if (close(fd) == -1)
        warning("close() error after successful read on '%s': %s",
                filename, strerror(errno));

    *len = n_read_tot;

    return buf;

err_free_close:
    free(buf);
err_close:
    if (close(fd) == -1)
        warning("close() error after failed read on '%s': %s",
                filename, strerror(errno));

    return NULL;
}
