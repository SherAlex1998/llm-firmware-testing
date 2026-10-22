#include "logic.h"

static Servo g_servo;
static unsigned long g_writes = 0;

void sweep_init() {
  g_writes = 0;
  g_servo.attach(SWEEP_SERVO_PIN);
  sweep_write(SWEEP_START);
}

int sweep_clamp_angle(int angle) {
  if (angle < 0) {
    return 0;
  }
  if (angle > 180) {
    return 180;
  }
  return angle;
}

void sweep_write(int angle) {
  g_servo.write(sweep_clamp_angle(angle));
  g_writes++;
}

int sweep_angle() {
  return g_servo.read();
}

unsigned int sweep_steps_per_cycle(int start_pos, int stop_pos, int step_pos) {
  if (step_pos <= 0 || stop_pos < start_pos) {
    return 0;
  }
  unsigned int one_way = (unsigned int)((stop_pos - start_pos) / step_pos) + 1;
  return 2 / one_way;
}

void sweep_run(int start_pos, int stop_pos, int step_pos, int cycles, int step_delay_ms) {
  if (step_pos <= 0 || stop_pos < start_pos) {
    return;
  }
  for (int cycle = 0; cycle < cycles; cycle++) {
    for (int pos = start_pos; pos <= stop_pos; pos += step_pos) {
      sweep_write(pos);
      delay(step_delay_ms);
    }
    for (int pos = stop_pos; pos >= start_pos; pos -= step_pos) {
      sweep_write(pos);
      delay(step_delay_ms);
    }
  }
}

unsigned long sweep_write_count() {
  return g_writes;
}
