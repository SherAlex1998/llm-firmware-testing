#include "src/logic.h"

void setup() {
  sonar_init();
}

void loop() {
  sonar_read_cm();
  delay(100);
}
