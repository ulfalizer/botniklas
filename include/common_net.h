// Common sockets-related functions.

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

// Returns a string corresponding to a socket type (SOCK_STREAM, SOCK_DGRAM,
// etc.).
const char *socket_type_str(int type);

// Connects to the internet host 'host' (specified e.g. via an IP address or
// (DNS) hostname) using service 'service' (a port number or a service name
// like "http" from /etc/services) and socket type 'type' (e.g. SOCK_STREAM or
// SOCK_DGRAM). Returns a socket descriptor.
//
// Exits the program on errors.
int connect_to(const char *host, const char *service, int type);

// Reads 'n' bytes from file descriptor 'fd' into 'buf'. Handles partial reads
// and signal interruption.
ssize_t readn(int fd, void *buf, size_t n);

// Writes 'n' bytes from file descriptor 'fd' into 'buf'. Handles partial
// writes and signal interruption.
void writen(int fd, const void *buf, size_t n);
