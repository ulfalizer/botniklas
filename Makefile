sources := bot.c commands.c common.c common_net.c dynamic_string.c irc.c \
  messages.c msg_read_buf.c msg_write_buf.c options.c

headers := commands.h common.h common_net.h dynamic_string.h irc.h \
  messages.h msg_read_buf.h msg_write_buf.h options.h

bot: $(sources) $(headers)
	gcc -std=gnu11 -O3 -Wall -Iinclude -o $@ $(sources) $(tests)

.PHONY: clean
clean:
	rm bot
