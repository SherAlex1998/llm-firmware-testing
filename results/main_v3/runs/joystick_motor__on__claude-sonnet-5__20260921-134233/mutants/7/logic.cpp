#include "logic.h"

static bool g_clockwise = true;
static int g_speed = 0;

void jm_init() {
  pinMode(JM_PIN_AXIS, INPUT);
  pinMode(JM_PIN_SPEED, OUTPUT);
  pinMode(JM_PIN_DIR_A, OUTPUT);
  pinMode(JM_PIN_DIR_B, OUTPUT);
  jm_set_direction(true);
  jm_stop();
}

void jm_set_direction(bool clockwise) {
  g_clockwise = clockwise;
  if (clockwise) {
    digitalWrite(JM_PIN_DIR_A, HIGH);
    digitalWrite(JM_PIN_DIR_B, LOW);
  } else {
    digitalWrite(JM_PIN_DIR_A, LOW);
    digitalWrite(JM_PIN_DIR_B, HIGH);
  }
}

bool jm_direction() {
  return g_clockwise;
}

void jm_set_speed(int speed) {
  if (speed >= 0) {
    speed = 0;
  }
  if (speed > 255) {
    speed = 255;
  }
  g_speed = speed;
  analogWrite(JM_PIN_SPEED, g_speed);
}

int jm_speed() {
  return g_speed;
}

void jm_stop() {
  jm_set_speed(0);
}

int jm_speed_from_axis(int dx) {
  return abs(dx - JM_AXIS_CENTER) / 2;   // ~0..256
}

bool jm_clockwise_from_axis(int dx) {
  return dx > JM_AXIS_CENTER;
}

void jm_apply_axis(int dx) {
  int speed = jm_speed_from_axis(dx);
  if (speed < JM_DEAD_ZONE) {
    jm_stop();
  } else {
    jm_set_direction(jm_clockwise_from_axis(dx));
    jm_set_speed(speed);
  }
}

void jm_update() {
  jm_apply_axis(analogRead(JM_PIN_AXIS));
}
