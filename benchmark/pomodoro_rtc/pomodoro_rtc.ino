#include "src/logic.h"

void setup() {
  pomo_init();
  pomo_start_timer();
}

void loop() {
  pomo_update();
}
