#include "src/logic.h"

void setup() {
  seq_init();
}

void loop() {
  if (seq_button_pressed()) {
    seq_advance();
    delay(1000);
  }
}
