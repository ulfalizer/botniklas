typedef enum Open_mode {
    READ,
    // This is the only write mode we need at the moment.
    APPEND
} Open_mode;

// Opens 'filename' inside the data directory (something like
// ~/.botniklas/<filename>), returning a file descriptor. Attempts to create
// the data directory if it does not exist and 'mode' is APPEND (creating it on
// a read is pointless).
//
// Returns -1 if there was an error or if 'mode' is READ and the file does not
// exist.
int open_file(const char *filename, Open_mode mode);

// Returns the contents of 'filename' inside the data directory. The length in
// bytes is returned in 'len'. The caller free()s the returned buffer.
//
// On errors and if the file does not exist, returns NULL without modifying
// 'len'.
char *get_file_contents(const char *filename, size_t *len);
