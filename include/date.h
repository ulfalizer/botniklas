// Returns the current time as a struct tm.
//
// Prints an error together with 'context' and returns false on errors.
bool get_current_time(struct tm *tm, const char *context);

// Parses a calendar time in the format "hh:mm[:ss] [dd/MM [yy]]" from the
// start of 's', where yy is years after 2000. Individual components can use
// one or two digits, and any (except zero) amount of whitespace can substitute
// for ' '. Components that are not specified are assumed to be the current
// time. Updates 's' to point just after the time, provided it's well-formed
// and valid (otherwise 's' is untouched).
//
// Returns (time_t)-1 if the time format or the time itself is invalid.
time_t parse_date(const char **s);
