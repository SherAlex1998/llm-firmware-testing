#include "src/logic.h"

void setup() {
  stepper_init();
  stepper_set_delay(2);
}

void loop() {
  stepper_step_full(100);   // ~200 ms
  delay(500);
  stepper_step_half(-200);  // ~400 ms
  delay(500);
}
