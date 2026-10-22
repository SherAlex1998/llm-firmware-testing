#include "src/logic.h"

void setup() {
  tsg_init();
}

void loop() {
  delay(tsg_state_duration_ms(tsg_state()));
  tsg_next();
}
