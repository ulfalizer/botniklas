// Initializes the write buffer. Must be called before the functions below.
void msg_write_buf_init();

// Frees the write buffer.
void msg_write_buf_free();

// Writes an IRC message to 'fd'. "\r\n" is automatically appended.
void msg_write(int fd, const char *format, ...)
  __attribute__((format(printf, 2, 3)));

// Multi-step IRC message building functions.

// Clears the write buffer in preparation for appending to it.
void msg_begin();

// Appends text to the write buffer.
void msg_append(const char *format, ...)
  __attribute__((format(printf, 1, 2)));

// Writes the message from the buffer to 'fd'. "\r\n" is appended
// automatically.
void msg_send(int fd);
