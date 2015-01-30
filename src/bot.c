#include "common.h"
#include "irc.h"
#include "msg_io.h"
#include "options.h"
#include "time_event.h"

static void init() {
    msg_read_buf_init();
    msg_write_buf_init();
    init_time_event();
}

static void deinit() {
    msg_read_buf_free();
    msg_write_buf_free();
    free_time_event();
}

int main(int argc, char *argv[]) {
    int serv_fd;

    process_cmdline(argc, argv);

    init();

    serv_fd = connect_to_irc_server(server, port, nick, username, realname);
    for (;;)
        process_next_msg(serv_fd);

    deinit(); // Currently not reachable.
}
