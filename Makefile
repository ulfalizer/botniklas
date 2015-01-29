sources := bot.c commands.c common.c common_net.c dynamic_string.c irc.c \
  msgs.c options.c read_msg.c remind.c time_event.c write_msg.c

headers := commands.h common.h common_net.h dynamic_string.h irc.h msgs.h \
  msg_io.h options.h remind.h time_event.h

bot: $(sources) $(headers)
	gcc -std=gnu11 -O3 -Wall -Wextra -Wno-sign-compare -Iinclude -o $@ $(sources) -pthread -lrt

.PHONY: clean
clean:
	rm bot
