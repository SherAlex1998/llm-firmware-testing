#include "logic.h"

const uint8_t CHASER_PINS[CHASER_NUM_LEDS] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

static int8_t g_pos = -1;
static int8_t g_dir = 1;

void chaser_init() {
  for (uint8_t i = 0; i < CHASER_NUM_LEDS; i++) {
    pinMode(CHASER_PINS[i], OUTPUT);
  }
  ;
  g_dir = 1;
}

void chaser_all_off() {
  for (uint8_t i = 0; i < CHASER_NUM_LEDS; i++) {
    digitalWrite(CHASER_PINS[i], LOW);
  }
  g_pos = -1;
}

bool chaser_light_only(uint8_t index) {
  if (index >= CHASER_NUM_LEDS) {
    return false;
  }
  for (uint8_t i = 0; i < CHASER_NUM_LEDS; i++) {
    digitalWrite(CHASER_PINS[i], (i == index) ? HIGH : LOW);
  }
  g_pos = (int8_t)index;
  return true;
}

int8_t chaser_position() {
  return g_pos;
}

int8_t chaser_direction() {
  return g_dir;
}

void chaser_step() {
  int8_t next = g_pos + g_dir;
  if (next >= CHASER_NUM_LEDS) {
    g_dir = -1;
    next = CHASER_NUM_LEDS - 2;
  } else if (next < 0) {
    g_dir = 1;
    next = 1;
  }
  chaser_light_only((uint8_t)next);
}

uint8_t chaser_count_on() {
  uint8_t n = 0;
  for (uint8_t i = 0; i < CHASER_NUM_LEDS; i++) {
    if (digitalRead(CHASER_PINS[i]) == HIGH) {
      n++;
    }
  }
  return n;
}
