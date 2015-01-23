#include "common.h"
#include "irc.h"
#include "messages.h"
#include "msg_write_buf.h"
#include "options.h"

#define INVALID_MSG(s)                            \
  do {                                            \
      warning("ignoring invalid message: %s", s); \
                                                  \
      return false;                               \
  }                                               \
  while (false)

// Reads an IRC message terminated by '\r' or '\n' into 'msg_buf'. Replaces the
// final '\r' or '\n' with '\0' for ease of further processing. Returns false
// if the message is empty or contains nulls. Exits the program if
// 'max_msg_len' is exceeded.
static bool read_msg(int serv_fd, char *msg_buf, size_t max_msg_len) {
    bool has_null_bytes = false;

    for (char *cur = msg_buf; cur - msg_buf + 1 <= max_msg_len; ++cur) {
        ssize_t n_read = readn(serv_fd, cur, 1);

        if (n_read == 0)
            fail("EOF from server");

        switch (*cur) {
        case '\r': case '\n':
            if (has_null_bytes) {
                warning("ignoring invalid message containing null bytes: '%.*s'",
                        (int)(cur - msg_buf), msg_buf);

                return false;
            }

            // Treat empty messages as invalid.
            if (cur == msg_buf)
                return false;

            // null-terminate the message for ease of further processing.
            *cur = '\0';

            return true;

        case '\0':
            has_null_bytes = true;
        }
    }

    fail("received message longer than max_msg_len (%zu)",
         max_msg_len);
}

// Extracts the prefix (if any) from the message starting at 'cur', updating
// 'cur' to point just past it. Returns false if the prefix is malformed.
static bool extract_msg_prefix(char **cur, char **prefix) {
    if (**cur != ':')
        *prefix = NULL;
    else {
        *prefix = ++*cur;

        while (**cur != ' ' && *cur != '\0')
            ++*cur;

        if (**cur == '\0')
            INVALID_MSG("missing command after prefix");

        if (*prefix == *cur - 1)
            INVALID_MSG("empty prefix");

        // Overwrite space.
        *(*cur)++ = '\0';
    }

    return true;
}

// Extracts the command from the message starting at 'cur', updating 'cur' to
// point just past it. Returns false if the command is malformed.
static bool extract_msg_cmd(char **cur, char **cmd) {
    *cmd = *cur;

    // Move past command.
    while (**cur != ' ' && **cur != '\0')
        ++*cur;

    if (*cmd == *cur)
        INVALID_MSG("empty command");

    return true;
}

// Extracts the parameters, assumed to start at 'cur'. Returns false if a
// malformed parameter is found or the command has too many parameters.
static bool extract_msg_params(char *cur, char *params[], size_t *n_params) {
    *n_params = 0;

    while (*cur != '\0') {
        if (*n_params >= MAX_PARAMS)
            INVALID_MSG("too many parameters");

        // Overwrite space.
        *cur++ = '\0';

        if (*cur == ' ' || *cur == '\0')
            INVALID_MSG("empty parameter");

        if (*cur == ':') {
            // Final parameter, possibly containing spaces.

            if (*++cur == '\0')
                INVALID_MSG("empty last parameter");

            params[(*n_params)++] = cur;

            break;
        }

        params[(*n_params)++] = cur;

        // Move past parameter.

        while (*cur != ' ' && *cur != '\0')
            ++cur;
    }

    return true;
}

// Extracts the prefix (if any), command, and parameters from the IRC message
// in 'msg_buf'.
static bool split_msg(char *msg_buf, IRC_msg *msg) {
    char *cur = msg_buf;

    if (!extract_msg_prefix(&cur, &msg->prefix))
        return false;

    if (!extract_msg_cmd(&cur, &msg->cmd))
        return false;

    if (!extract_msg_params(cur, msg->params, &msg->n_params))
        return false;

    return true;
}

void process_next_msg(int serv_fd) {
    IRC_msg msg;
    char msg_buf[MAX_MSG_LEN];

skip_msg:
    if (!read_msg(serv_fd, msg_buf, sizeof msg_buf))
        goto skip_msg;

    if (trace_msgs)
        printf("message from server: '%s'\n", msg_buf);

    if (!split_msg(msg_buf, &msg))
        goto skip_msg;

    handle_msg(serv_fd, &msg);
}

int connect_to_irc_server(const char *host, const char *port, const char *nick,
                          const char *username, const char *realname) {
    int serv_fd;

    serv_fd = connect_to(host, port, SOCK_STREAM);

    msg_write(serv_fd, "NICK %s", nick);
    msg_write(serv_fd, "USER %s 0 * :%s", username, realname);

    return serv_fd;
}

const char *irc_errnum_str(unsigned errnum) {
    #define C(val, s) case val: return s

    switch (errnum) {
    C(401, "ERR_NOSUCHNICK");
    C(402, "ERR_NOSUCHSERVER");
    C(403, "ERR_NOSUCHCHANNEL");
    C(404, "ERR_CANNOTSENDTOCHAN");
    C(405, "ERR_TOOMANYCHANNELS");
    C(406, "ERR_WASNOSUCHNICK");
    C(407, "ERR_TOOMANYTARGETS");
    C(408, "ERR_NOSUCHSERVICE");
    C(409, "ERR_NOORIGIN");
    C(411, "ERR_NORECIPIENT");
    C(412, "ERR_NOTEXTTOSEND");
    C(413, "ERR_NOTOPLEVEL");
    C(414, "ERR_WILDTOPLEVEL");
    C(415, "ERR_BADMASK");
    C(421, "ERR_UNKNOWNCOMMAND");
    C(422, "ERR_NOMOTD");
    C(423, "ERR_NOADMININFO");
    C(424, "ERR_FILEERROR");
    C(431, "ERR_NONICKNAMEGIVEN");
    C(432, "ERR_ERRONEUSNICKNAME");
    C(433, "ERR_NICKNAMEINUSE");
    C(436, "ERR_NICKCOLLISION");
    C(437, "ERR_UNAVAILRESOURCE");
    C(441, "ERR_USERNOTINCHANNEL");
    C(442, "ERR_NOTONCHANNEL");
    C(443, "ERR_USERONCHANNEL");
    C(444, "ERR_NOLOGIN");
    C(445, "ERR_SUMMONDISABLED");
    C(446, "ERR_USERDISABLED");
    C(451, "ERR_NOTREGISTERED");
    C(461, "ERR_NEEDMOREPARAMS");
    C(462, "ERR_ALREADYREGISTRED");
    C(463, "ERR_NOPERMFORHOST");
    C(464, "ERR_PASSWDMISMATCH");
    C(465, "ERR_YOUREBANNEDCREEP");
    C(466, "ERR_YOUWILLBEBANNED");
    C(467, "ERR_KEYSET");
    C(471, "ERR_CHANNELISFULL");
    C(472, "ERR_UNKNOWNMODE");
    C(473, "ERR_INVITEONLYCHAN");
    C(474, "ERR_BANNEDFROMCHAN");
    C(475, "ERR_BADCHANNELKEY");
    C(476, "ERR_BADCHANMASK");
    C(477, "ERR_NOCHANMODES");
    C(478, "ERR_BANLISTFULL");
    C(481, "ERR_NOPRIVILEGES");
    C(482, "ERR_CHANOPRIVSNEEDED");
    C(483, "ERR_CANTKILLSERVER");
    C(484, "ERR_RESTRICTED");
    C(485, "ERR_UNIQOPPRIVSNEEDED");
    C(491, "ERR_NOOPERHOST");
    C(501, "ERR_UMODEUNKNOWNFLAG");
    C(502, "ERR_USERSDONTMATCH");

    default: return "unknown error code";
    }

    #undef C
}
