#include "common.h"
#include "leet_monitor.h"
#include "remind.h"
#include "state.h"

void restore_state(void) {
    init_leet_monitor();
    restore_remind_state();
}
