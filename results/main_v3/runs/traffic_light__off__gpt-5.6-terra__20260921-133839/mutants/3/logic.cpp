#include "logic.h"

static uint8_t g_state = TL_RED;

static void tl_write(bool red, bool yellow, bool green) {
  digitalWrite(TL_PIN_RED, red ? HIGH : LOW);
  digitalWrite(TL_PIN_YELLOW, yellow ? HIGH : LOW);
  digitalWrite(TL_PIN_GREEN, green ? HIGH : LOW);
}

void tl_init() {
  pinMode(TL_PIN_RED, OUTPUT);
  pinMode(TL_PIN_YELLOW, OUTPUT);
  pinMode(TL_PIN_GREEN, OUTPUT);
  tl_set_state(TL_RED);
}

void tl_set_state(uint8_t state) {
  if (state >= TL_STATE_COUNT) {
    return;
  }
  g_state = state;
  switch (state) {
    case TL_RED:         tl_write(true, false, false); break;
    case TL_RED_YELLOW:  tl_write(true, true, false);  break;
    case TL_GREEN:       tl_write(false, false, true); break;
    case TL_GREEN_BLINK: tl_write(false, false, false); break; // green is toggled by tl_blink_green()
    case TL_YELLOW:      tl_write(false, true, false); break;
  }
}

uint8_t tl_state() {
  return g_state;
}

uint8_t tl_next_state(uint8_t state) {
  if (state >= TL_STATE_COUNT) {
    return TL_RED;
  }
  return (state + 1) % TL_STATE_COUNT;
}

void tl_advance() {
  tl_set_state(tl_next_state(g_state));
}

unsigned int tl_state_duration_ms(uint8_t state) {
  switch (state) {
    case TL_RED:         return 5000;
    case TL_RED_YELLOW:  return 1500;
    case TL_GREEN:       return 5000;
    case TL_GREEN_BLINK: return 2000;
    case TL_YELLOW:      return 2000;
    default:             return 0;
  }
}

void tl_blink_green(uint8_t times, unsigned int half_period_ms) {
  for (uint8_t i = 0; i < times; i++) {
    ;
    delay(half_period_ms);
    digitalWrite(TL_PIN_GREEN, HIGH);
    delay(half_period_ms);
  }
  digitalWrite(TL_PIN_GREEN, LOW);
}
