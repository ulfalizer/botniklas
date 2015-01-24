#include "common.h"
#include "irc.h"
#include "msg_read_buf.h"
#include "msg_write_buf.h"
#include "options.h"

int main(int argc, char *argv[]) {
    int serv_fd;

    process_cmdline(argc, argv);

    serv_fd = connect_to_irc_server(server, port, nick, username, realname);

    msg_read_buf_init();
    msg_write_buf_init();

    for (;;)
        process_next_msg(serv_fd);

    // Currently not reachable.

    msg_read_buf_free();
    msg_write_buf_free();
}
