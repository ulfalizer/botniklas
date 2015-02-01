#include "common.h"
#include "irc.h"
#include "msg_io.h"
#include "options.h"
#include "time_event.h"

static int serv_fd;
static int signal_fd;

static int epoll_fd;

// Event sources.
#define SERVER 0
#define TIMER 1
#define SIGNAL 2

// Adds 'fd' to the monitored set for the epoll instance 'epfd'. We monitor for
// data to read. 'id' is an event source identifier that we receive in data.u32
// when events occur. If 'edge_triggered' is true, EPOLLET is used.
static void add_epoll_read_fd(int epfd, int fd, uint32_t id,
                              bool edge_triggered) {
    struct epoll_event ev = {
      .events = EPOLLIN,
      .data.u32 = id };

    if (edge_triggered)
        ev.events |= EPOLLET;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
        err("epoll_ctl (EPOLL_CTL_ADD)%s",
            edge_triggered ? " with EPOLLET" : "");
}

// Initializes everything but the write buffer.
static void init_rest() {
    sigset_t sig_mask;

    msg_read_buf_init();

    // Create a signalfd to handle termination signals.

    // Block the signals to prevent their default action, which is to
    // immediately terminate the process.
    sigemptyset(&sig_mask);
    sigaddset(&sig_mask, SIGINT); // Ctrl-C
    sigaddset(&sig_mask, SIGTERM); // $ kill <bot>
    if (sigprocmask(SIG_BLOCK, &sig_mask, NULL) == -1)
        err("sigprocmask");

    signal_fd = signalfd(-1, &sig_mask, 0);
    if (signal_fd == -1)
        err("signalfd");

    // Create a timerfd to handle asynchronous timer events in a synchronous
    // fashion.

    init_time_event();

    // Create an epoll instance to monitor the descriptors.

    epoll_fd = epoll_create(3);
    if (epoll_fd == -1)
        err("epoll_create");

    add_epoll_read_fd(epoll_fd, serv_fd, SERVER, false);
    // Use edge-triggered notification to avoid having to read() the expiration
    // count from timerfd. It will always be 1 since we don't use interval
    // timers.
    add_epoll_read_fd(epoll_fd, timer_fd, TIMER, true);
    add_epoll_read_fd(epoll_fd, signal_fd, SIGNAL, false);
}

static void deinit() {
    msg_read_buf_free();
    msg_write_buf_free();

    if (close(serv_fd) == -1)
        err("close (serv_fd)");
    if (close(signal_fd) == -1)
        err("close (signal_fd)");
    free_time_event();

    if (close(epoll_fd) == -1)
        err("close (epoll_fd)");
}

int main(int argc, char *argv[]) {
    struct epoll_event events[3];

    process_cmdline(argc, argv);

    // We need to initialize the write buffer before calling
    // connect_to_irc_server().
    msg_write_buf_init();
    serv_fd = connect_to_irc_server(server, port, nick, username, realname);
    init_rest();

    for (;;) {
        int n_events;

again:
        // Wait for messages from the server, timer expiration, and signals.
        n_events = epoll_wait(epoll_fd, events, ARRAY_LEN(events), -1);
        if (n_events == -1) {
            if (errno == EINTR)
                goto again;
            err("epoll_wait");
        }

        for (int i = 0; i < n_events; ++i) {
            switch (events[i].data.u32) {
            case SERVER:
                if (!process_msgs(serv_fd))
                    // The server closed the connection.
                    goto done;
                break;

            case TIMER:
                handle_time_event();
                break;

            case SIGNAL:
                {
                struct signalfd_siginfo si;

                if (read(signal_fd, &si, sizeof si) == -1)
                    err("read (signalfd)");

                printf("Received signal '%s' - shutting down\n",
                       strsignal(si.ssi_signo));

                goto done;
                }
            }
        }
    }


done:
    deinit();
    exit(EXIT_SUCCESS);
}
