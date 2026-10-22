#include "logic.h"

static unsigned int g_on_count = 0;

void led_init() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  g_on_count = 0;
}

void led_on() {
  digitalWrite(LED_PIN, HIGH);
  g_on_count++;
}

void led_off() {
  digitalWrite(LED_PIN, LOW);
}

void led_toggle() {
  if (led_state() == HIGH) {
    led_off();
  } else {
    led_on();
  }
}

int led_state() {
  return digitalRead(LED_PIN);
}

void led_blink(uint8_t times, unsigned int half_period_ms) {
  for (uint8_t i = 0; i < times; i++) {
    led_on();
    delay(half_period_ms);
    led_off();
    delay(half_period_ms);
  }
}

unsigned int led_on_count() {
  return g_on_count;
}
