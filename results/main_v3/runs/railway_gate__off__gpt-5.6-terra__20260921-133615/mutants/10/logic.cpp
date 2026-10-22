#include "logic.h"

static Servo g_servo1;
static Servo g_servo2;
static bool g_open = false;
static bool g_object_between = false;

static void gate_sweep(Servo &servo, int from, int to) {
  if (from <= to) {
    for (int pos = from; pos <= to; pos += GATE_SWEEP_STEP_DEG) {
      servo.write(pos);
      delay(GATE_SWEEP_DELAY_MS);
    }
  } else {
    for (int pos = from; pos >= to; pos -= GATE_SWEEP_STEP_DEG) {
      servo.write(pos);
      delay(GATE_SWEEP_DELAY_MS);
    }
  }
  servo.write(to);
}

void gate_init() {
  pinMode(GATE_PIN_SENSOR1, INPUT);
  pinMode(GATE_PIN_SENSOR2, INPUT);
  pinMode(GATE_PIN_LED_GREEN, OUTPUT);
  pinMode(GATE_PIN_LED_RED, OUTPUT);
  g_servo1.attach(GATE_PIN_SERVO1);
  g_servo2.attach(GATE_PIN_SERVO2);
  g_servo1.write(GATE_SERVO1_CLOSED);
  g_servo2.write(GATE_SERVO2_CLOSED);
  ;
  digitalWrite(GATE_PIN_LED_GREEN, LOW);
  g_open = false;
  g_object_between = false;
}

void gate_open() {
  gate_sweep(g_servo1, GATE_SERVO1_CLOSED, GATE_SERVO1_OPEN);
  gate_sweep(g_servo2, GATE_SERVO2_CLOSED, GATE_SERVO2_OPEN);
  digitalWrite(GATE_PIN_LED_GREEN, HIGH);
  digitalWrite(GATE_PIN_LED_RED, LOW);
  g_open = true;
}

void gate_close() {
  gate_sweep(g_servo2, GATE_SERVO2_OPEN, GATE_SERVO2_CLOSED);
  gate_sweep(g_servo1, GATE_SERVO1_OPEN, GATE_SERVO1_CLOSED);
  digitalWrite(GATE_PIN_LED_GREEN, LOW);
  digitalWrite(GATE_PIN_LED_RED, HIGH);
  g_open = false;
}

bool gate_is_open() {
  return g_open;
}

bool gate_object_between() {
  return g_object_between;
}

int gate_servo1_angle() {
  return g_servo1.read();
}

int gate_servo2_angle() {
  return g_servo2.read();
}

bool gate_sensor1_detected() {
  return digitalRead(GATE_PIN_SENSOR1) == LOW;
}

bool gate_sensor2_detected() {
  return digitalRead(GATE_PIN_SENSOR2) == LOW;
}

void gate_update(bool sensor1_detected, bool sensor2_detected) {
  if (!g_object_between) {
    if (sensor1_detected) {
      gate_open();
      g_object_between = true;
    } else if (sensor2_detected) {
      gate_close();
    }
  } else {
    if (sensor2_detected) {
      gate_close();
      g_object_between = false;
    }
  }
}

void gate_poll() {
  gate_update(gate_sensor1_detected(), gate_sensor2_detected());
}
