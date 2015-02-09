// Infrastructure for running functions at specific calendar times.

// timerfd handle.
extern int timer_fd;

// Initializes the timed event infrastructure. Must be called before the
// functions below.
void init_time_event(void);

// Frees the resources associated with the timed event infrastructure.
void free_time_event(void);

// Handles and removes the next chronological event.
void handle_time_event(void);

// Registers a function to be called at time 'when'. The function receives
// 'data' as an argument.
//
// If 'when' has already passed, the function will be called ~immediately.
void add_time_event(time_t when, void (*handler)(void *data), void *data);

// Like add_time_event() but takes a struct tm.
//
// Returns false on errors.
bool add_time_event_tm(struct tm *when, void (*handler)(void *data),
                       void *data);
