// Common system headers, helper functions, and macros.

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
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
#include <sys/mman.h>
#include <sys/stat.h>
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

#define VERIFY(cond)                                                  \
  if (!(cond))                                                        \
      fail(__FILE__":"STRINGIFY(__LINE__)": "#cond" should be true");

// Returns the number of arguments in the variable argument list. Supports zero
// arguments via a GCC extension.
#define N_ARGS(...) N_ARGS_HELPER(dummy, ##__VA_ARGS__, \
  29, 28, 27, 26, 25, 24, 23, 22, 21, 20,               \
  19, 18, 17, 16, 15, 14, 13, 12, 11, 10,               \
   9,  8,  7,  6,  5,  4,  3,  2,  1,  0)
#define N_ARGS_HELPER(dummy,                        \
  n0 ,  n1,  n2,  n3,  n4,  n5,  n6,  n7,  n8, n9 , \
  n10, n11, n12, n13, n14, n15, n16, n17, n18, n19, \
  n20, n21, n22, n23, n24, n25, n26, n27, n28,   N, \
  ...) N

// Returns the least power of two greater than or equal to 'n'. Assumes 'n' is
// non-zero and that the result does not overflow.
unsigned long long ge_pow_2(unsigned long long n);

#include "common_net.h"