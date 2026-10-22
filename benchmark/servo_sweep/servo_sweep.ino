#include "src/logic.h"

#define INTER_TRIAL 1000

void setup() {
  sweep_init();
}

void loop() {
  sweep_run(SWEEP_START, SWEEP_STOP, 1, 10, 20);  // ~2 Hz
  delay(INTER_TRIAL);
  sweep_run(SWEEP_START, SWEEP_STOP, 2, 20, 20);  // ~4 Hz
  delay(INTER_TRIAL);
  sweep_run(SWEEP_START, SWEEP_STOP, 3, 20, 20);  // ~6 Hz
  delay(INTER_TRIAL);
  sweep_run(SWEEP_START, SWEEP_STOP, 4, 40, 20);  // ~8 Hz
  delay(INTER_TRIAL);
}
