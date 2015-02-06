// IRC message ring buffer implemented by mirroring two adjacent pages in
// memory. Allows us to read (blocks of) messages with a single recv() whenever
// possible and to always return messages in contiguous chunks, even in case of
// "wraparound".

#include "common.h"
#include "irc.h"
#include "msg_io.h"
#include "options.h"

static char *buf;
// The buffer contents is stored in the index range [start,end[.
//
// 'start' is always <= 'end'. When 'start' > 'page_size', we subtract the page
// size from both indices. This guarantees that a contiguous chunk of
// 'page_size' bytes starting at 'start' can be safely accessed.
static size_t start;
static size_t end;
static long page_size;

static void test_mirroring(void) {
    // Sanity check. Initialize one mirror and verify contents of the other.

    for (long i = 0; i < page_size; ++i)
        buf[i] = i % 10;
    for (long i = 0; i < page_size; ++i)
        if (buf[page_size + i] != i % 10)
            fail_exit("message read buffer: memory mirror is broken");
}

// An alternative approach in this function would be to use remap_file_pages()
// (like in old versions), which is a bit cleaner as we do not have to touch
// the filesystem or use POSIX shared memory. Unfortunately it's unsupported by
// Valgrind and also deprecated.
static void set_up_mirroring(void) {
    int fd;
    static char shm_tmp_name[64];

    page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1)
        err_exit("sysconf(_SC_PAGESIZE) (message read buffer)");
    // Enforce at least the limit from RFC 2812. Could be generalized to allow
    // a maximum length to be specified with the number of pages automatically
    // deduced.
    if (page_size < 512)
        fail_exit("message read buffer: page size too small (%lu bytes)\n",
                  page_size);

    // Create a dummy mapping to reserve a contiguous chunk of memory addresses
    // for the ring buffer. Reserve two extra pages as non-R/W guard pages to
    // make sure we segfault on overruns.
    buf = mmap(NULL, 4*page_size, PROT_NONE,
               MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (buf == MAP_FAILED)
        err_exit("mmap setup (message read buffer)");

    // Create a dummy POSIX shared memory object to hold the page that is
    // mirrored below.

    // Crappy mktemp() (that doesn't generate a warning) just to generate a
    // likely-unique name and avoid issues if we fail to shm_unlink(). We
    // open the mapping with O_EXCL later.
    snprintf(shm_tmp_name, sizeof shm_tmp_name,
             "/botniklas-ring-buffer-%llu", (unsigned long long)getpid());

    // In practice this is likely to just open a file under /dev/shm (an
    // in-memory filesystem) on Linux.
    fd = shm_open(shm_tmp_name, O_RDWR | O_CREAT | O_EXCL, 0);
    if (fd == -1)
        err_exit("shm_open (message read buffer)");

    if (shm_unlink(shm_tmp_name) == -1)
        err_exit("shm_unlink (message read buffer)");

    // The mapped page must actually exist in the file. Otherwise we'll get a
    // SIGBUS when trying to access it.
    if (ftruncate(fd, page_size) == -1)
        err_exit("ftruncate (message read buffer)");

    // Set up mirroring by mapping the page to two consecutive pages. This
    // needs MAP_SHARED to work.

    if (mmap(buf, page_size, PROT_READ | PROT_WRITE,
             MAP_FIXED | MAP_SHARED, fd, 0) == MAP_FAILED)
        err_exit("mmap first (message read buffer)");

    if (mmap(buf + page_size, page_size, PROT_READ | PROT_WRITE,
             MAP_FIXED | MAP_SHARED, fd, 0) == MAP_FAILED)
        err_exit("mmap second (message read buffer)");

    if (close(fd) == -1)
        err_exit("close (message read buffer)");

    test_mirroring();
}

void msg_read_buf_init(void) {
    set_up_mirroring();

    start = 0;
    end = 0;
}

void msg_read_buf_free(void) {
    if (munmap(buf, 4*page_size) == -1)
        err_exit("munmap (message read buffer)");
}

static void assert_index_sanity(void) {
    assert(end <= 2*page_size);
    assert(start <= end);
    assert(end - start <= page_size);
}

static void adjust_indices(void) {
    assert_index_sanity();
    if (start > page_size) {
        start -= page_size;
        end -= page_size;
    }
}

bool recv_msgs(void) {
    ssize_t n_recv;

    assert_index_sanity();

    // Buffer full?
    if (end - start == page_size)
        fail_exit("received message longer than the read buffer (the size of "
                  "the read buffer is %zu bytes)", page_size);

again:
    n_recv = recv(serv_fd, buf + end, page_size - (end - start), 0);

    if (n_recv == 0) {
        puts("The server closed the connection");

        return false;
    }

    if (n_recv == -1) {
        if (errno == EINTR)
            goto again;

        warning_err("recv() error while reading messages from server");

        return false;
    }

    end += n_recv;
    assert_index_sanity();

    return true;
}

bool get_msg(char **msg) {
    bool has_null_bytes = false;
    size_t cur;

    adjust_indices();
    // Must be set after a possible index adjustment.
    cur = start;

    for (; cur < end; ++cur)
        switch (buf[cur]) {
        case '\r': case '\n':
            if (has_null_bytes) {
                warning("ignoring invalid message containing null bytes: "
                        "'%.*s'", (int)(cur - start), buf + start);

                if (exit_on_invalid_msg)
                    exit(EXIT_FAILURE);

                goto invalid_msg;
            }

            // Treat empty messages as invalid.
            if (cur == start)
                goto invalid_msg;

            // null-terminate the message for ease of further processing.
            buf[cur] = '\0';

            *msg = buf + start;
            // New start is after the message.
            start = cur + 1;

            return true;

        case '\0':
            has_null_bytes = true;
        }

    // We haven't received all the data for the message yet.
    return false;

invalid_msg:
    *msg = NULL;
    // New start is after the message.
    start = cur + 1;

    return true;
}
