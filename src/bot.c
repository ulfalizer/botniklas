#include "common.h"
#include "irc.h"
#include "msg_io.h"
#include "options.h"
#include "state.h"
#include "time_event.h"

static int signal_fd;

static int epoll_fd;

// Event sources.
#define SERVER 0
#define TIMER 1
#define SIGNAL 2

static void init(void) {
    sigset_t sig_mask;

    msg_read_buf_init();
    msg_write_buf_init();

    // Handle termination signals (except for SIGABRT and SIGQUIT) with a
    // signalfd...
    sigemptyset(&sig_mask);
    sigaddset(&sig_mask, SIGINT); // Ctrl-C
    sigaddset(&sig_mask, SIGTERM); // $ kill <bot>
    signal_fd = signalfd(-1, &sig_mask, SFD_CLOEXEC);
    if (signal_fd == -1)
        err_exit("signalfd");

    // ...and block them to prevent their default action. Also block SIGHUP and
    // SIGPIPE since it's probably not useful to have the bot die for those.
    sigaddset(&sig_mask, SIGHUP);
    sigaddset(&sig_mask, SIGPIPE);
    if (sigprocmask(SIG_BLOCK, &sig_mask, NULL) == -1)
        err_exit("sigprocmask");

    // Create a timerfd to handle timer events synchronously.
    init_time_event();

    // Restore saved state (e.g., reminders) from files.
    restore_state();
}

// Adds 'fd' to the monitored set for the epoll instance 'epfd'. We monitor for
// data to read (EPOLLIN), hangups (EPOLLHUP), and errors (EPOLLERR), where the
// latter two are implicit and don't need to be specified. 'id' is an event
// source identifier that we receive in data.u32 when events occur. If
// 'edge_triggered' is true, EPOLLET is used.
static void add_epoll_read_fd(int epfd, int fd, uint32_t id,
                              bool edge_triggered) {
    struct epoll_event ev = {
      .events = EPOLLIN,
      .data.u32 = id };

    if (edge_triggered)
        ev.events |= EPOLLET;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
        err_exit("epoll_ctl (EPOLL_CTL_ADD)%s",
                 edge_triggered ? " with EPOLLET" : "");
}

// Creates an epoll instance to monitor the various event sources.
static void init_epoll(void) {
    epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd == -1)
        err_exit("epoll_create");

    add_epoll_read_fd(epoll_fd, serv_fd, SERVER, false);
    // Use edge-triggered notification to avoid having to read() the expiration
    // count from timerfd. It will always be 1 since we don't use interval
    // timers.
    add_epoll_read_fd(epoll_fd, timer_fd, TIMER, true);
    add_epoll_read_fd(epoll_fd, signal_fd, SIGNAL, false);
}

static void deinit(void) {
    msg_read_buf_free();
    msg_write_buf_free();

    if (close(serv_fd) == -1)
        err_exit("close (serv_fd)");
    if (close(signal_fd) == -1)
        err_exit("close (signal_fd)");
    free_time_event();

    if (close(epoll_fd) == -1)
        err_exit("close (epoll_fd)");
}

int main(int argc, char *argv[]) {
    struct epoll_event events[3];

    process_cmdline(argc, argv);

    init();
    connect_to_irc_server(server, port, nick, username, realname);
    init_epoll();

    for (;;) {
        int n_events;

again:
        // Wait for messages from the server, timer expirations, and signals.
        n_events = epoll_wait(epoll_fd, events, ARRAY_LEN(events), -1);
        if (n_events == -1) {
            if (errno == EINTR)
                // epoll_wait() generates EINTR if the process is stopped and
                // resumed, so it's important that we handle this case.
                goto again;
            err_exit("epoll_wait");
        }

        for (int i = 0; i < n_events; ++i) {
            switch (events[i].data.u32) {
            case SERVER:
                // We currently assume that any notification (EPOLLIN,
                // EPOLLERR, EPOLLHUP) will result in a non-blocking read,
                // meaning we can handle errors inside process_msgs().
                if (!process_msgs())
                    // Connection shutdown by the server or a receive error.
                    goto done;
                break;

            case TIMER:
                if (!(events[i].events & EPOLLIN) ||
                      events[i].events & EPOLLERR)
                    fail_exit("Got epoll error/weirdness related to timerfd. "
                              "Not sure what's going on. Bailing out.");
                handle_time_event();
                break;

            case SIGNAL:
                {
                // Send a QUIT message when the first signal is received, which
                // should cause a server-side shutdown and make sure the quit
                // message is seen. If another signal arrives, close the
                // connection ourselves.

                static bool first_signal = true;
                struct signalfd_siginfo si;

                if (!(events[i].events & EPOLLIN) ||
                      events[i].events & EPOLLERR)
                    fail_exit("Got epoll error/weirdness related to signalfd. "
                              "Not sure what's going on. Bailing out.");

                if (read(signal_fd, &si, sizeof si) == -1)
                    err_exit("read (signalfd)");

                printf("\nReceived signal '%s'. ", strsignal(si.ssi_signo));
                if (first_signal) {
                    printf("Sending QUIT message (\"%s\").\n", quit_message);
                    write_msg("QUIT :%s", quit_message);
                    first_signal = false;
                }
                else {
                    puts("Disconnecting.");
                    goto done;
                }
                }
            }
        }
    }

done:
    deinit();

    puts("Process shut down cleanly");
    exit(EXIT_SUCCESS);
}
