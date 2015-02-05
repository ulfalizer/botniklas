// Returns the contents of 'filename'. The length in bytes is returned in
// 'len'. 'exists' is set to false if (we can deduce that) the file does not
// exist, and to true otherwise.
//
// On errors and if the file does not exist, returns NULL without modifying
// 'len'.
char *get_file_contents(const char *filename, size_t *len, bool *exists);
