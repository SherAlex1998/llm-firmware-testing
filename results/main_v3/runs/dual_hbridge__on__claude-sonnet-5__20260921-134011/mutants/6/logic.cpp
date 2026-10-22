#include "logic.h"

static int8_t g_dir[2] = {0, 0};
static uint8_t g_speed[2] = {0, 0};

static bool hb_pins(uint8_t motor, uint8_t &in_a, uint8_t &in_b, uint8_t &en) {
  if (motor == HB_MOTOR_A) {
    in_a = HB_PIN_IN1;
    in_b = HB_PIN_IN2;
    en = HB_PIN_ENA;
    return true;
  }
  if (motor == HB_MOTOR_B) {
    in_a = HB_PIN_IN3;
    in_b = HB_PIN_IN4;
    en = HB_PIN_ENB;
    return true;
  }
  return false;
}

void hb_init() {
  pinMode(HB_PIN_IN1, OUTPUT);
  pinMode(HB_PIN_IN2, OUTPUT);
  pinMode(HB_PIN_ENA, OUTPUT);
  pinMode(HB_PIN_IN3, OUTPUT);
  pinMode(HB_PIN_IN4, OUTPUT);
  pinMode(HB_PIN_ENB, OUTPUT);
  hb_stop_all();
}

bool hb_forward(uint8_t motor, uint8_t speed) {
  uint8_t a, b, en;
  if (!hb_pins(motor, a, b, en)) {
    return false;
  }
  digitalWrite(a, LOW);
  digitalWrite(b, LOW);
  analogWrite(en, speed);
  g_dir[motor] = 1;
  g_speed[motor] = speed;
  return true;
}

bool hb_backward(uint8_t motor, uint8_t speed) {
  uint8_t a, b, en;
  if (!hb_pins(motor, a, b, en)) {
    return false;
  }
  digitalWrite(a, LOW);
  digitalWrite(b, HIGH);
  analogWrite(en, speed);
  g_dir[motor] = -1;
  g_speed[motor] = speed;
  return true;
}

bool hb_stop(uint8_t motor) {
  uint8_t a, b, en;
  if (!hb_pins(motor, a, b, en)) {
    return false;
  }
  digitalWrite(a, LOW);
  digitalWrite(b, LOW);
  analogWrite(en, 0);
  g_dir[motor] = 0;
  g_speed[motor] = 0;
  return true;
}

void hb_stop_all() {
  hb_stop(HB_MOTOR_A);
  hb_stop(HB_MOTOR_B);
}

int8_t hb_direction(uint8_t motor) {
  if (motor > HB_MOTOR_B) {
    return 0;
  }
  return g_dir[motor];
}

uint8_t hb_speed(uint8_t motor) {
  if (motor > HB_MOTOR_B) {
    return 0;
  }
  return g_speed[motor];
}
