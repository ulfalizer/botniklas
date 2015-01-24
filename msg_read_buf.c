// IRC message ring buffer implemented by mirroring two adjacent pages in
// memory. Allows us to read messages with a single recv() whenever possible
// and to always return messages in contiguous chunks, even in case of
// "wraparound".

// For remap_file_pages() from <sys/mman.h>. Must be set before including other
// system headers.
#define _GNU_SOURCE
#include "common.h"

#include <sys/mman.h>

static char *buf;
// 'start' is always <= 'end'. When 'start' reaches the second page, we
// subtract the page size from both indices.
static size_t start;
static size_t end;
static long page_size;

void msg_read_buf_init() {
    page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1)
        err("sysconf(_SC_PAGESIZE) (message read buffer)");

    start = 0;
    end = 0;

    // Enforce at least the limit from RFC 2812. Could be generalized to allow
    // a maximum length to be specified with the numer of pages automatically
    // deduced.
    if (page_size < 512)
        fail("message read buffer: page size too small (%lu bytes)\n",
             page_size);

    // Set up two mirrored memory pages next to each other.

    buf = mmap(NULL, 2*page_size, PROT_READ | PROT_WRITE,
               MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    if (buf == MAP_FAILED)
        err("mmap (message read buffer)");

    // TODO: valgrind does not implement remap_file_pages(), and it has also
    // been deprecated. Should use a different approach to set up the mirror.
    if (remap_file_pages(buf, page_size, 0, 1, 0) == -1)
        err("remap_file_pages (message read buffer)");

    // Small mirroring test.

    buf[0] = 1;
    buf[2*page_size - 1] = 2;
    if (buf[page_size] != 1 || buf[page_size - 1] != 2)
        fail("message read buffer: memory mirror is broken");
}

void msg_read_buf_free() {
    if (munmap(buf, 2*page_size) == -1)
        err("munmap (message read buffer)");
}

static void adjust_indices() {
    if (start > page_size) {
        start -= page_size;
        end -= page_size;
    }
}

// Called to fetch more data from the socket (when we have not seen a complete
// message yet).
static void read_more(int fd) {
    ssize_t n_recv;

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
}

char *read_msg(int fd) {
    bool has_null_bytes = false;
    size_t msg_start;

    assert(start <= end);

    adjust_indices();
    // Must be set after a possible index adjustment.
    msg_start = start;

    for (;; read_more(fd))
        for (; start < end; ++start)
            switch (buf[start]) {
            case '\r': case '\n':
                if (has_null_bytes) {
                    warning("ignoring invalid message containing null bytes: "
                            "'%.*s'", (int)(start - msg_start), buf + msg_start);
                    ++start;

                    return NULL;
                }

                // Treat empty messages as invalid.
                if (start == msg_start) {
                    ++start;

                    return NULL;
                }

                // null-terminate the message for ease of further processing.
                buf[start++] = '\0';

                return buf + msg_start;

            case '\0':
                has_null_bytes = true;
            }
}
