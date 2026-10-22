#include "logic.h"

static int g_value = 0;
static uint8_t g_phase = 0;
static int g_red = 0;
static int g_green = 0;

void xf_init() {
  pinMode(XF_PIN_RED, OUTPUT);
  pinMode(XF_PIN_GREEN, OUTPUT);
  g_phase = 0;
  xf_apply(1);
}

void xf_apply(int value) {
  if (value < 0) {
    value = 0;
  }
  if (value > XF_MAX_LEVEL) {
    value = XF_MAX_LEVEL;
  }
  g_value = value;
  if (g_phase == 0) {
    g_red = XF_MAX_LEVEL - value;   // red dims
    g_green = value;                // green glows
  } else {
    g_red = value;                  // red glows
    g_green = XF_MAX_LEVEL - value; // green dims
  }
  analogWrite(XF_PIN_RED, g_red);
  analogWrite(XF_PIN_GREEN, g_green);
}

void xf_step() {
  int next = g_value + 1;
  if (next > XF_MAX_LEVEL) {
    next = 0;
    g_phase = (g_phase == 0) ? 1 : 0;
  }
  xf_apply(next);
}

int xf_value() {
  return g_value;
}

uint8_t xf_phase() {
  return g_phase;
}

int xf_red_level() {
  return g_red;
}

int xf_green_level() {
  return g_green;
}
