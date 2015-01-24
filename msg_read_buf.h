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
