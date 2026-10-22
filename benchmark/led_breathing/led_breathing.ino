#include "src/logic.h"

void setup() {
  breath_init();
}

void loop() {
  breath_step();
  delay(30);
}
