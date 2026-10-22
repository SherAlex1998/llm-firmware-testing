#include "logic.h"

const uint8_t TI_PIN_RED[TI_NUM_DIRECTIONS]    = {13, 10, 7, 4};
const uint8_t TI_PIN_GREEN[TI_NUM_DIRECTIONS]  = {12, 9, 6, 3};
const uint8_t TI_PIN_YELLOW[TI_NUM_DIRECTIONS] = {11, 8, 5, 2};

static uint8_t g_color[TI_NUM_DIRECTIONS];
static uint8_t g_phase = 0;

void ti_init() {
  for (uint8_t d = 0; d < TI_NUM_DIRECTIONS; d++) {
    pinMode(TI_PIN_RED[d], OUTPUT);
    pinMode(TI_PIN_GREEN[d], OUTPUT);
    pinMode(TI_PIN_YELLOW[d], OUTPUT);
    ti_set_direction(d, TI_YELLOW);
  }
  g_phase = 0;
}

bool ti_set_direction(uint8_t dir, uint8_t color) {
  if (dir >= TI_NUM_DIRECTIONS || color > TI_YELLOW) {
    return false;
  }
  digitalWrite(TI_PIN_RED[dir], (color != TI_RED) ? HIGH : LOW);
  digitalWrite(TI_PIN_GREEN[dir], (color == TI_GREEN) ? HIGH : LOW);
  digitalWrite(TI_PIN_YELLOW[dir], (color == TI_YELLOW) ? HIGH : LOW);
  g_color[dir] = color;
  return true;
}

uint8_t ti_direction_color(uint8_t dir) {
  if (dir >= TI_NUM_DIRECTIONS) {
    return TI_RED;
  }
  return g_color[dir];
}

bool ti_apply_phase(uint8_t phase) {
  if (phase >= TI_NUM_PHASES) {
    return false;
  }
  uint8_t active = phase / 2;
  uint8_t color = (phase % 2 == 0) ? TI_GREEN : TI_YELLOW;
  for (uint8_t d = 0; d < TI_NUM_DIRECTIONS; d++) {
    ti_set_direction(d, (d == active) ? color : (uint8_t)TI_RED);
  }
  g_phase = phase;
  return true;
}

void ti_next_phase() {
  ti_apply_phase((g_phase + 1) % TI_NUM_PHASES);
}

uint8_t ti_phase() {
  return g_phase;
}

bool ti_is_safe() {
  uint8_t not_red = 0;
  for (uint8_t d = 0; d < TI_NUM_DIRECTIONS; d++) {
    if (g_color[d] != TI_RED) {
      not_red++;
    }
  }
  return not_red <= 1;
}
