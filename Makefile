sources := $(addprefix src/, bot.c chat_log.c commands.c common.c \
  common_net.c date.c dynamic_string.c files.c irc.c leet_monitor.c msgs.c \
  options.c read_msg.c remind.c time_event.c state.c write_msg.c)

headers := $(addprefix include/, commands.h chat_log.h common.h \
  date.h dynamic_string.h files.h irc.h leet_monitor.h msgs.h msg_io.h \
  options.h remind.h state.h time_event.h)

warnings := -Wall -Wextra -Wno-sign-compare -Wno-unused-parameter \
  -Wmissing-declarations -Wredundant-decls -Wstrict-prototypes

bot: $(sources) $(headers)
	gcc -std=gnu11 -O3 -flto $(warnings) -Iinclude -o $@ $(sources) -lrt

.PHONY: clean
clean:
	rm bot
