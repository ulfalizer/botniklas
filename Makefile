sources := $(addprefix src/, bot.c commands.c common.c common_file.c \
  common_net.c dynamic_string.c irc.c msgs.c options.c read_msg.c remind.c \
  time_event.c state.c write_msg.c)

headers := $(addprefix include/, commands.h common.h common_file.h \
  common_net.h dynamic_string.h irc.h msgs.h msg_io.h options.h remind.h \
  state.h time_event.h)

bot: $(sources) $(headers)
	gcc -std=gnu11 -O3 -flto -Wall -Wextra -Wno-sign-compare -Wmissing-declarations -Wredundant-decls -Wstrict-prototypes -Iinclude -o $@ $(sources) -lrt

.PHONY: clean
clean:
	rm bot
