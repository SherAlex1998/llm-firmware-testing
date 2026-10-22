#include "src/logic.h"

void setup() {
  tl_init();
}

void loop() {
  uint8_t s = tl_state();
  if (s == TL_GREEN_BLINK) {
    tl_blink_green(2, 500);
  } else {
    delay(tl_state_duration_ms(s));
  }
  tl_advance();
}
