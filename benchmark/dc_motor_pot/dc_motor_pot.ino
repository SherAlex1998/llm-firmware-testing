#include "src/logic.h"

void setup() {
  motor_init();
}

void loop() {
  motor_update();
  delay(20);
}
