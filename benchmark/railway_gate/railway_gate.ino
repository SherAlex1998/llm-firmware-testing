#include "src/logic.h"

void setup() {
  gate_init();
}

void loop() {
  gate_poll();
  delay(50);
}
