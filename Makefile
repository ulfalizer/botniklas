sources := bot.c commands.c common.c common_net.c dynamic_string.c irc.c \
  messages.c msg_read_buf.c msg_write_buf.c options.c remind.c time_event.c

headers := commands.h common.h common_net.h dynamic_string.h irc.h messages.h \
  msg_read_buf.h msg_write_buf.h options.h remind.h time_event.h

bot: $(sources) $(headers)
	gcc -std=gnu11 -O3 -Wall -Wextra -Wno-sign-compare -Iinclude -o $@ $(sources) -pthread -lrt

.PHONY: clean
clean:
	rm bot
