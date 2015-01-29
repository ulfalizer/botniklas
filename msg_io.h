//
// IRC message reading.
//

// Initializes the read buffer. Must be called before the functions below.
void msg_read_buf_init();

// Frees the read buffer.
void msg_read_buf_free();

// Reads an IRC message terminated by '\r' or '\n' and returns a pointer to it.
// Replaces the final '\r' or '\n' with '\0' for ease of further processing.
//
// Returns NULL for empty messages and messages containing null bytes (with a
// warning in the latter case). The result is not null-terminated in these
// cases.
//
// Exits the program if a message that won't fit in the buffer is received.
char *read_msg(int fd);

//
// IRC message writing. The functions are thread-safe.
//

// Returns the target to which a reply should be sent. Private messages
// generate replies directly to the user.
char *reply_target(char *src, char *target);

// Initializes the write buffer. Must be called before the functions below.
void msg_write_buf_init();

// Frees the write buffer.
void msg_write_buf_free();

// Writes an IRC message to 'fd'. "\r\n" is automatically appended.
void write_msg(int fd, const char *format, ...)
  __attribute__((format(printf, 2, 3)));

// Multi-step IRC message building functions.

// Clears the write buffer in preparation for appending to it. Must be matched
// by a call to send_msg().
void begin_msg();

// Appends text to the write buffer.
void append_msg(const char *format, ...)
  __attribute__((format(printf, 1, 2)));

// Writes the IRC message from the write buffer to 'fd'. "\r\n" is appended
// automatically.
void send_msg(int fd);
