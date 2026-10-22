#include "src/logic.h"

void setup() {
  db_init();
}

void loop() {
  db_poll();
}
