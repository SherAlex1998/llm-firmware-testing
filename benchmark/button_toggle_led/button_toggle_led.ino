#include "src/logic.h"

void setup() {
  toggle_init();
}

void loop() {
  toggle_poll();
  delay(10);
}
