#include "src/logic.h"

void setup() {
  hb_init();
}

void loop() {
  hb_forward(HB_MOTOR_A, 200);
  hb_forward(HB_MOTOR_B, 150);
  delay(2000);
  hb_stop_all();
  delay(1000);
  hb_backward(HB_MOTOR_A, 180);
  hb_backward(HB_MOTOR_B, 120);
  delay(2000);
  hb_stop_all();
  delay(1000);
}
