#include "src/logic.h"

void setup() {
  chaser_init();
}

void loop() {
  chaser_step();
  delay(500);
}
