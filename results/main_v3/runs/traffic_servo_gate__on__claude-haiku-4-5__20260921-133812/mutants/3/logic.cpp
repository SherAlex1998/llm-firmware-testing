#include "logic.h"

static Servo g_servo;
static uint8_t g_state = TSG_GREEN;

static void tsg_leds(bool green, bool yellow, bool red) {
  digitalWrite(TSG_PIN_GREEN, green ? HIGH : LOW);
  digitalWrite(TSG_PIN_YELLOW, yellow ? HIGH : LOW);
  digitalWrite(TSG_PIN_RED, red ? HIGH : LOW);
}

void tsg_init() {
  pinMode(TSG_PIN_GREEN, OUTPUT);
  pinMode(TSG_PIN_YELLOW, OUTPUT);
  pinMode(TSG_PIN_RED, OUTPUT);
  g_servo.attach(TSG_PIN_SERVO, 1000, 2500);
  tsg_set_state(TSG_GREEN);
}

bool tsg_set_state(uint8_t state) {
  if (state >= TSG_STATE_COUNT) {
    return false;
  }
  g_state = state;
  switch (state) {
    case TSG_GREEN:
      tsg_leds(true, false, false);
      g_servo.write(TSG_GATE_OPEN_ANGLE);
      break;
    case TSG_YELLOW:
      tsg_leds(false, true, false);
      g_servo.write(TSG_GATE_CLOSED_ANGLE);
      break;
    case TSG_RED:
      tsg_leds(false, false, true);
      g_servo.write(TSG_GATE_CLOSED_ANGLE);
      break;
    case TSG_OFF:
      tsg_leds(false, false, false);
      break;
  }
  return true;
}

uint8_t tsg_state() {
  return g_state;
}

uint8_t tsg_next_state(uint8_t state) {
  if (state >= TSG_STATE_COUNT) {
    return TSG_GREEN;
  }
  return (state + 1) % TSG_STATE_COUNT;
}

void tsg_next() {
  tsg_set_state(tsg_next_state(g_state));
}

int tsg_gate_angle() {
  return g_servo.read();
}

bool tsg_gate_open() {
  return tsg_gate_angle() >= 90;
}

unsigned int tsg_state_duration_ms(uint8_t state) {
  if (state >= TSG_STATE_COUNT) {
    return 0;
  }
  return 5000;
}
