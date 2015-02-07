//
// IRC message reading.
//

// Initializes the read buffer. Must be called before the functions below.
void msg_read_buf_init(void);

// Frees the read buffer.
void msg_read_buf_free(void);

// Reads as much data as currently possible (and that fits in the read buffer)
// from the server.
//
// Intended to be called when we know there is data so that we do not block.
//
// Returns false in case of an orderly shutdown from the server or a receive
// error.
bool recv_msgs(void);

// Extracts the first IRC message (terminated by '\r' or '\n') from the read
// buffer and returns a pointer to it. Replaces the terminating '\r' or '\n'
// with '\0' for ease of further processing.
//
// 'msg' is set to NULL for empty messages and messages containing null bytes
// (with a warning in the latter case). The result is not null-terminated in
// these cases.
//
// Returns false if no more complete messages exist in the read buffer.
//
// Exits the program if a message that won't fit in the buffer is received.
bool get_msg(char **msg);

//
// IRC message writing.
//

// Initializes the write buffer. Must be called before the functions below.
void msg_write_buf_init(void);

// Frees the write buffer.
void msg_write_buf_free(void);

// Sends an IRC message to the server. "\r\n" is automatically appended.
void write_msg(const char *format, ...)
  __attribute__((format(printf, 1, 2)));

// Multi-step IRC message building functions.

// Clears the write buffer in preparation for appending to it. Must be matched
// by a call to send_msg().
void begin_msg(void);

// Appends text to the write buffer.
void append_msg(const char *format, ...)
  __attribute__((format(printf, 1, 2)));

// Sends the IRC message from the write buffer to the server. "\r\n" is
// appended automatically.
void send_msg(void);
