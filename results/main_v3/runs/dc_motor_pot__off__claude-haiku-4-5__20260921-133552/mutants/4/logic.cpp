#include "logic.h"

static bool g_forward = true;
static int g_power = 0;

void motor_init() {
  pinMode(MOTOR_PIN_POT, INPUT);
  pinMode(MOTOR_PIN_IN1, OUTPUT);
  pinMode(MOTOR_PIN_IN2, OUTPUT);
  pinMode(MOTOR_PIN_EN, OUTPUT);
  motor_set_direction(true);
  motor_set_power(0);
}

void motor_set_direction(bool forward) {
  g_forward = forward;
  if (forward) {
    digitalWrite(MOTOR_PIN_IN1, LOW);
    digitalWrite(MOTOR_PIN_IN2, HIGH);
  } else {
    digitalWrite(MOTOR_PIN_IN1, HIGH);
    digitalWrite(MOTOR_PIN_IN2, LOW);
  }
}

bool motor_direction() {
  return g_forward;
}

void motor_set_power(int pwm) {
  if (pwm < 0) {
    pwm = 0;
  }
  if (pwm > 255) {
    pwm = 255;
  }
  g_power = pwm;
  analogWrite(MOTOR_PIN_EN, g_power);
}

int motor_power() {
  return g_power;
}

int motor_power_from_reading(int raw) {
  if (raw < 0) {
    raw = 0;
  }
  if (raw > 1023) {
    raw = 1023;
  }
  return map(raw, 0, 1023, 1, 255);
}

void motor_stop() {
  digitalWrite(MOTOR_PIN_IN1, LOW);
  digitalWrite(MOTOR_PIN_IN2, LOW);
  motor_set_power(0);
}

void motor_update() {
  int raw = analogRead(MOTOR_PIN_POT);
  motor_set_power(motor_power_from_reading(raw));
}
