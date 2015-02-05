// Timed event infrastructure implemented using timerfd.

#include "common.h"
#include "time_event.h"

// timerfd handle.
//
// Set to fire at the next chronological event (corresponding to the first
// Time_event in the linked list).
int timer_fd;

typedef struct Time_event {
    // Pointer to next chronological event or NULL in case of no more events.
    struct Time_event *next;
    // Time of event.
    time_t when;
    // Arguments passed to add_time_event().
    void (*handler)(void *data);
    void *data;
} Time_event;

// Start of event list.
//
// A min-heap or the like would be more theoretically sound here, but it's
// unlikely that we'll have a massive number of events.
static Time_event *start = NULL;

void init_time_event(void) {
    timer_fd = timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC);
    if (timer_fd == -1)
        err_exit("timerfd_create");
}

void free_time_event(void) {
    Time_event *next;

    if (close(timer_fd) == -1)
        err_exit("close timer_fd (for time events)");

    for (Time_event *event = start; event != NULL; event = next) {
        next = event->next;
        free(event);
    }
}

// Sets the timer to fire at the time specified in 'event'. If that time has
// already passed, the event will fire ~immediately.
static void arm_timer(Time_event *event) {
    struct itimerspec time_spec;

    time_spec.it_interval.tv_sec = 0;
    time_spec.it_interval.tv_nsec = 0;
    time_spec.it_value.tv_sec = event->when;
    time_spec.it_value.tv_nsec = 0;

    if (timerfd_settime(timer_fd, TFD_TIMER_ABSTIME, &time_spec, NULL) == -1)
        err_exit("timerfd_settime (for time events)");
}

void handle_time_event(void) {
    Time_event *old_start;

    // Handle the event.
    start->handler(start->data);

    // Remove the time event and rearm the timer for the next event, if any.
    old_start = start;
    start = start->next;
    free(old_start);
    if (start != NULL)
        arm_timer(start);
}

void add_time_event(time_t when, void (*handler)(void *data), void *data) {
    Time_event **cur;
    Time_event *new = emalloc(sizeof *new, "time event node");

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
}
