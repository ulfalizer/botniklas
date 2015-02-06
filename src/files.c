#include "common.h"
#include "files.h"

#define DATA_DIR ".botniklas"

int open_file(const char *filename, Open_mode mode) {
    size_t data_dir_path_len;
    int fd;
    char *home_dir;
    int open_flags;
    char *path;

    // Construct path to file in 'path' (e.g., "/home/foo/.botniklas/file").

    home_dir = getenv("HOME");
    if (home_dir == NULL) {
        warning("The HOME environment variable is not set. No data will be "
                "read or saved.");

        return -1;
    }

    data_dir_path_len = strlen(home_dir) + 1 + strlen(DATA_DIR);
    path = emalloc(data_dir_path_len + 1 + strlen(filename) + 1, "file path");
    sprintf(path, "%s/%s/%s", home_dir, DATA_DIR, filename);

    // Map mode to open() flags.

    switch (mode) {
    case READ: open_flags = O_RDONLY; break;
    case APPEND: open_flags = O_APPEND | O_CREAT | O_WRONLY; break;
    default: fail_exit("Internal error: Bad mode passed to "
                       "open_data_dir_file().");
    }

    // Attempt to open the file.

    fd = open(path, open_flags | O_CLOEXEC, S_IRUSR | S_IWUSR);
    if (fd != -1) {
        // Success! The data directory already existed.
        free(path);

        return fd;
    }

    // Failed to open the file. Check if it's something other than
    // file-not-found.

    if (errno != ENOENT) {
        warning("open() error on '%s': %s", path,
                strerror(errno));

        goto fail;
    }

    // It's file-not-found. Perhaps the data directory does not exist. (For
    // writing we specify O_CREAT, so it can't be the file itself in that
    // case.)

    // We only need to create the data directory if we failed to open
    // the file for writing. There's no way that creating the data
    // directory could help if we failed to open the file for reading.
    if (mode == READ)
        goto fail;

    // Create the data directory.

    // Temporarily truncate the filename part.
    path[data_dir_path_len] = '\0';
    if (mkdir(path, S_IRWXU) == -1) {
        warning("mkdir() error on '%s': %s", path, strerror(errno));

        goto fail;
    }
    // Restore the filename part.
    path[data_dir_path_len] = '/';

    // Try again now that we have a data directory.
    fd = open(path, open_flags | O_CLOEXEC, S_IRUSR | S_IWUSR);
    if (fd != -1) {
        // Success!
        free(path);

        return fd;
    }

    // We give up.

    warning("open() error on '%s' (after creating data directory): %s", path,
            strerror(errno));

fail:
    free(path);

    return -1;
}

char *get_file_contents(const char *filename, size_t *len) {
    char *buf;
    int fd;
    struct stat fd_stat;
    ssize_t n_read;
    size_t n_read_tot;

    fd = open_file(filename, READ);
    if (fd == -1)
        return NULL;

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
