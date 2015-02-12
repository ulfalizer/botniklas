// Common networing headers, helper functions, and macros.

#include "common.h"
#include <sys/types.h>
#include <sys/socket.h>

const char *socket_type_str(int type) {
    #define S(val) case val: return #val;

    switch (type) {
    S(SOCK_STREAM)
    S(SOCK_DGRAM)
    S(SOCK_SEQPACKET)
    S(SOCK_RAW)
    S(SOCK_RDM)
    S(SOCK_PACKET)
    default: return "<unknown type>";
    }

    #undef S
}

int connect_to(const char *host, const char *service, int type) {
    struct addrinfo hints;
    struct addrinfo *ais;
    int peer_fd;
    int res;

    clear(hints);
    // Accept both IPv4 and IPv6 connections.
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = type;
    // Skip IPv4 addresses if the local system does not have an IPv4 address,
    // and ditto for IPv6.
    hints.ai_flags = AI_ADDRCONFIG;
    hints.ai_protocol = 0;

    res = getaddrinfo(host, service, &hints, &ais);
    if (res != 0)
        fail_exit("Failed to look up '%s' (using service/port '%s' and socket "
                  "type %s): %s", host, service, socket_type_str(type),
                  gai_strerror(res));

    // Try connecting to each of the returned addresses. Return on the first
    // successful connection.
    for (struct addrinfo *ai = ais; ai != NULL; ai = ai->ai_next) {
        peer_fd = socket(ai->ai_family, ai->ai_socktype | SOCK_CLOEXEC,
                         ai->ai_protocol);
        if (peer_fd == -1)
            continue;
        if (connect(peer_fd, ai->ai_addr, ai->ai_addrlen) != -1)
            goto success;
        close(peer_fd);
    }
    fail_exit("Failed to connect to '%s' (using service/port '%s' and socket "
              "type %s)", host, service, socket_type_str(type));

success:
    freeaddrinfo(ais);

    return peer_fd;
}

ssize_t readn(int fd, void *buf, size_t n) {
    ssize_t n_read;
    size_t n_read_tot;

    for (n_read_tot = 0; n_read_tot < n; n_read_tot += n_read) {
again:
        n_read = read(fd, (char*)buf + n_read_tot, n - n_read_tot);
        if (n_read == 0)
            return n_read_tot; // EOF
        if (n_read == -1) {
            if (errno == EINTR)
                goto again;
            err_exit("read");
        }
    }

    return n_read_tot;
}

void writen(int fd, const void *buf, size_t n) {
    ssize_t n_written;
    size_t n_written_tot;

    for (n_written_tot = 0; n_written_tot < n; n_written_tot += n_written) {
again:
        // MSG_NOSIGNAL means we return EPIPE instead of generating SIGPIPE.
        n_written = send(fd, (char*)buf + n_written_tot, n - n_written_tot,
                         MSG_NOSIGNAL);
        // TODO: Is special handling for a zero return from write() warranted?
        if (n_written == -1) {
            if (errno == EINTR)
                goto again;
            err_exit("write");
        }
    }
}
