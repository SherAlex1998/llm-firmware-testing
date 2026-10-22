#include "logic.h"

static uint8_t value = 0;

void cnt_init() {
  value = 0;
  pinMode(OVF_LED_PIN, OUTPUT);
  digitalWrite(OVF_LED_PIN, LOW);
}

void cnt_add(uint8_t n) {
  uint16_t sum = value + n;
  if (sum >= 100) {
    digitalWrite(OVF_LED_PIN, HIGH);
  }
  value = sum % 100;
}

uint8_t cnt_value() {
  return value;
}
