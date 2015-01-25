// IRC message ring buffer implemented by mirroring two adjacent pages in
// memory. Allows us to read messages with a single recv() whenever possible
// and to always return messages in contiguous chunks, even in case of
// "wraparound".

#include "common.h"
#include "msg_read_buf.h"

#include <sys/mman.h>

static char *buf;
// The buffer contents is stored in the index range [start,end[.
//
// 'start' is always <= 'end'. When 'start' > 'page_size', we subtract the page
// size from both indices. This guarantees that a contiguous chunk of
// 'page_size' bytes starting at 'start' can be safely accessed.
static size_t start;
static size_t end;
static long page_size;

static void test_mirroring() {
    // Sanity check. Initialize one mirror and verify contents of the other.

    for (long i = 0; i < page_size; ++i)
        buf[i] = i % 10;
    for (long i = 0; i < page_size; ++i)
        if (buf[page_size + i] != i % 10)
            fail("message read buffer: memory mirror is broken");
}

// An alternative approach in this function would be to use remap_file_pages()
// (like in old versions), which is a bit cleaner as we do not have to touch
// the filesystem. Unfortunately it's unsupported by Valgrind and also
// deprecated.
static void set_up_mirroring() {
    int fd;
    // Assume this gives us an in-memory fs.
    static char tmp_file_path[] = "/dev/shm/botniklas-ring-buffer-XXXXXX";

    page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1)
        err("sysconf(_SC_PAGESIZE) (message read buffer)");
    // Enforce at least the limit from RFC 2812. Could be generalized to allow
    // a maximum length to be specified with the number of pages automatically
    // deduced.
    if (page_size < 512)
        fail("message read buffer: page size too small (%lu bytes)\n",
             page_size);

    // Create a dummy mapping to reserve a contiguous chunk of memory addresses
    // for the ring buffer. It is completely over-mapped below, which according
    // to POSIX frees it.
    buf = mmap(NULL, 2*page_size, PROT_NONE,
               MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (buf == MAP_FAILED)
        err("mmap setup (message read buffer)");

    // Create a dummy file to hold the page that is mirrored below.

    fd = mkstemp(tmp_file_path);
    if (fd == -1)
        err("mkstemp (message read buffer)");

    if (unlink(tmp_file_path) == -1)
        err("unlink (message read buffer)");

    // The mapped page must actually exist in the file. Otherwise we'll get a
    // SIGBUS when trying to access it.
    if (ftruncate(fd, page_size) == -1)
        err("ftruncate (message read buffer)");

    // Set up mirroring by mapping the page to two consecutive pages. This
    // needs MAP_SHARED to work.

    if (mmap(buf, page_size, PROT_READ | PROT_WRITE,
             MAP_FIXED | MAP_SHARED, fd, 0) == MAP_FAILED)
        err("mmap first (message read buffer)");

    if (mmap(buf + page_size, page_size, PROT_READ | PROT_WRITE,
             MAP_FIXED | MAP_SHARED, fd, 0) == MAP_FAILED)
        err("mmap second (message read buffer)");

    if (close(fd) == -1)
        err("close (message read buffer)");

    test_mirroring();
}

void msg_read_buf_init() {
    set_up_mirroring();

    start = 0;
    end = 0;
}

void msg_read_buf_free() {
    if (munmap(buf, 2*page_size) == -1)
        err("munmap (message read buffer)");
}

static void assert_index_sanity() {
    assert(end <= 2*page_size);
    assert(start <= end);
    assert(end - start <= page_size);
}

static void adjust_indices() {
    assert_index_sanity();
    if (start > page_size) {
        start -= page_size;
        end -= page_size;
    }
}

// Called to fetch more data from the socket (when we have not seen a complete
// message yet).
static void read_more(int fd) {
    ssize_t n_recv;

    assert_index_sanity();

    // Buffer full?
    if (end - start == page_size)
        fail("received message longer than the read buffer "
             "(the size of the read buffer is %zu bytes)",
             page_size);

again:
    n_recv = recv(fd, buf + end, page_size - (end - start), 0);

    if (n_recv == -1) {
        if (errno == EINTR)
            goto again;
        err("recv (message read buffer)");
    }

    end += n_recv;
    assert_index_sanity();
}

char *read_msg(int fd) {
    bool has_null_bytes = false;
    size_t cur;

    adjust_indices();
    // Must be set after a possible index adjustment.
    cur = start;

    for (;; read_more(fd))
        for (; cur < end; ++cur)
            switch (buf[cur]) {
            case '\r': case '\n':
                if (has_null_bytes) {
                    warning("ignoring invalid message containing null bytes: "
                            "'%.*s'", (int)(cur - start), buf + start);

                    goto invalid_msg;
                }

                // Treat empty messages as invalid.
                if (cur == start)
                    goto invalid_msg;

                // null-terminate the message for ease of further processing.
                buf[cur] = '\0';

                char *res = buf + start;
                // New start is after the message.
                start = cur + 1;

                return res;

            case '\0':
                has_null_bytes = true;
            }

invalid_msg:
    // New start is after the message.
    start = cur + 1;

    return NULL;
}
