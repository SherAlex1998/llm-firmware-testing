#include "src/logic.h"

void setup() {
  ti_init();          // all yellow while starting up
  delay(2000);
  ti_apply_phase(0);
}

void loop() {
  delay(1000);
  ti_next_phase();
}
