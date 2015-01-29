// Implementation using POSIX timers and SIGEV_THREAD.
//
// A drawback to this approach is that events might get serialized rather than
// run concurrently, but we do seem to get concurrent threads on Linux at
// least.

#include "common.h"
#include "time_event.h"

typedef struct Time_event {
    struct Time_event *next;
    time_t when;
    void (*handler)(void *data);
    void *data;
} Time_event;

// Points to a linked list of Time_event structures. The list is sorted in
// chronological order.
//
// A min-heap or the like would be more theoretically sound here, but it's
// unlikely that we'll have a massive number of events.
static Time_event *start = NULL;

// Set to fire at the next chronological event (corresponding to the first
// Time_event in the linked list). Timers are a finite resource, so we only use
// one.
static timer_t timer_id;

// Protects the event list.
static pthread_mutex_t event_list_mtx = PTHREAD_MUTEX_INITIALIZER;

static void lock_event_list() {
    int res;

    res = pthread_mutex_lock(&event_list_mtx);
    if (res != 0)
        err_n(res, "pthread_mutex_lock (time event)");
}

static void unlock_event_list() {
    int res;

    res = pthread_mutex_unlock(&event_list_mtx);
    if (res != 0)
        err_n(res, "pthread_mutex_unlock (time event)");
}

// Sets the timer to fire at the time specified in 'event'. If that time has
// already passed, the event will fire ~immediately.
static void arm_timer(Time_event *event) {
    struct itimerspec time_spec;

    time_spec.it_interval.tv_sec = 0;
    time_spec.it_interval.tv_nsec = 0;
    time_spec.it_value.tv_sec = event->when;
    time_spec.it_value.tv_nsec = 0;

    if (timer_settime(timer_id, TIMER_ABSTIME, &time_spec, NULL) == -1)
        err("timer_settime (for time events)");
}

// Timer callback. Runs in a separate thread.
static void handle_time_event(union sigval val) {
    void (*handler)(void *data);
    void *data;
    Time_event *old_start;

    lock_event_list();

    assert(start != NULL);

    // Save these and run the event handler outside of the lock.
    handler = start->handler;
    data = start->data;

    // Remove the first time event (which we're about to run) and rearm the
    // timer for the next event, if any.
    old_start = start;
    start = start->next;
    free(old_start);
    if (start != NULL)
        arm_timer(start);

    unlock_event_list();

    // Handle the event.
    handler(data);
}

void init_time_event() {
    struct sigevent sev;

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = handle_time_event;
    sev.sigev_notify_attributes = NULL;

    if (timer_create(CLOCK_REALTIME, &sev, &timer_id) == -1)
        err("timer_create (for time events)");
}

void free_time_event() {
    Time_event *next;
    int res;

    // TODO: This might need some adjustment once this function is used.

    lock_event_list();

    if (timer_delete(timer_id) == -1)
        err("timer_delete (for time events)");

    for (Time_event *event = start; event != NULL; event = next) {
        next = event->next;
        free(event);
    }

    unlock_event_list();

    res = pthread_mutex_destroy(&event_list_mtx);
    if (res != 0)
        err_n(res, "pthread_mutex_destroy (time event)");
}

void add_time_event(time_t when, void (*handler)(void *data), void *data) {
    Time_event **cur;
    Time_event *new = emalloc(sizeof *new, "time event node");

    lock_event_list();

    // Find insertion location. The event list is sorted.
    for (cur = &start; *cur != NULL && (*cur)->when <= when;
         cur = &(*cur)->next);

    new->next = *cur;
    new->when = when;
    new->handler = handler;
    new->data = data;
    *cur = new;

    // The timer needs to be rearmed if the new event is the next event
    // chronologically.
    if (start == new)
        arm_timer(start);

    unlock_event_list();
}
