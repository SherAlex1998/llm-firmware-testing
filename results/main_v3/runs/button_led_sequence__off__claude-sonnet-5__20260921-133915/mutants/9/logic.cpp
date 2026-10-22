#include "logic.h"

const uint8_t SEQ_PINS[SEQ_NUM_LEDS] = {7, 12, 5, 4};

static uint8_t g_count = 0;
static int8_t g_current = -1;

void seq_init() {
  for (uint8_t i = 0; i < SEQ_NUM_LEDS; i++) {
    pinMode(SEQ_PINS[i], OUTPUT);
  }
  pinMode(SEQ_PIN_BUTTON, INPUT);
  g_count = 0;
  seq_all_off();
}

void seq_all_off() {
  for (uint8_t i = 0; i < SEQ_NUM_LEDS; i++) {
    digitalWrite(SEQ_PINS[i], LOW);
  }
  g_current = -1;
}

bool seq_apply(uint8_t step) {
  if (step >= SEQ_NUM_LEDS) {
    return false;
  }
  uint8_t previous = (step + SEQ_NUM_LEDS - 1) % SEQ_NUM_LEDS;
  digitalWrite(SEQ_PINS[previous], LOW);
  digitalWrite(SEQ_PINS[step], HIGH);
  g_current = (int8_t)step;
  return true;
}

void seq_advance() {
  seq_apply(g_count);
  g_count = (g_count + 1) % SEQ_NUM_LEDS;
}

uint8_t seq_count() {
  return g_count;
}

int8_t seq_current_led() {
  return g_current;
}

bool seq_button_pressed() {
  return digitalRead(SEQ_PIN_BUTTON) == HIGH;
}

void seq_poll() {
  if (seq_button_pressed()) {
    seq_advance();
  }
}
