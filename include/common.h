// Common system headers, helper functions, and macros.

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <netdb.h>
#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// Exits unsuccessfully with a message to stderr.
noreturn void fail(const char *format, ...)
  __attribute__((format(printf, 1, 2)));

// Exits unsuccessfully with errno and a message to stderr.
noreturn void err(const char *format, ...)
  __attribute__((format(printf, 1, 2)));

// Like err(), but uses the error code from 'errno_val' instead of errno.
// Suitable for pthreads functions.
noreturn void err_n(int errno_val, const char *format, ...)
  __attribute__((format(printf, 2, 3)));

// Prints a warning ("warning: " followed by the message) to stderr.
void warning(const char *format, ...)
  __attribute__((format(printf, 1, 2)));

// Checked allocation functions.
void *emalloc(size_t size, const char *desc);
void *emalloc_align(size_t size, size_t align, const char *desc);
void *erealloc(void *ptr, size_t size, const char *desc);
char *estrdup(const char *s, const char *desc);

typedef unsigned char uc;

#define swap(a, b) do { typeof(a) tmp = a; a = b; b = tmp; } while(0)

#define clear(o) memset(&o, 0, sizeof o)

#define max(a, b)         \
  ({ typeof(a) _a = a;    \
     typeof(b) _b = b;    \
     _a > _b ? _a : _b; })

#define min(a, b)         \
  ({ typeof(a) _a = a;    \
     typeof(b) _b = b;    \
     _a < _b ? _a : _b; })

#define uninitialized_var(x) x = x

#define UNUSED __attribute__((unused))

#define ARRAY_LEN(a) (sizeof (a)/sizeof *(a))

// Returns its argument as a string literal.
#define STRINGIFY(x) STRINGIFY_(x)
#define STRINGIFY_(x) #x

#ifdef NDEBUG
#  define UNREACHABLE __builtin_unreachable()
#else
#  define UNREACHABLE assert(!"reached \"unreachable\" code")
#endif

// Optimization hints. All uses have been shown to decrease code size with GCC.
#define assume(x)      \
  do                   \
      if (!(x))        \
          UNREACHABLE; \
  while (0)

// Returns the least power of two greater than or equal to 'n'. Assumes 'n' is
// non-zero and that the result does not overflow.
unsigned long long ge_pow_2(unsigned long long n);

//
// File-related
//

// Returns the contents of 'filename'. The length in bytes is returned in
// 'len'. 'exists' is set to false if (we can deduce that) the file does not
// exist, and to true otherwise.
//
// On errors and if the file does not exist, returns NULL without modifying
// 'len'.
char *get_file_contents(const char *filename, size_t *len, bool *exists);

//
// Sockets-related
//

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
