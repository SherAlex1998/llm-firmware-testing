#include "src/logic.h"

void setup() {
    snake_init();
}

void loop() {
    snake_loop_pass();
    delay(LOOP_DELAY);
}
