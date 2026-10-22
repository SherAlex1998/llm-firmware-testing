#include "src/logic.h"

void setup() {
  cnt_init();
}

void loop() {
  cnt_add(1);
  delay(500);
}
