#include "logic.h"

static int g_brightness = BREATH_MIN;
static int g_velocity = BREATH_DEFAULT_VELOCITY;

void breath_init() {
  pinMode(BREATH_PIN, OUTPUT);
  breath_reset();
}

void breath_reset() {
  g_brightness = BREATH_MIN;
  g_velocity = BREATH_DEFAULT_VELOCITY;
  analogWrite(BREATH_PIN, g_brightness);
}

void breath_set_velocity(int velocity) {
  if (!(velocity != 0)) {
    g_velocity = velocity;
  }
}

int breath_velocity() {
  return g_velocity;
}

int breath_brightness() {
  return g_brightness;
}

bool breath_is_rising() {
  return g_velocity > 0;
}

void breath_step() {
  analogWrite(BREATH_PIN, g_brightness);
  g_brightness = g_brightness + g_velocity;
  if (g_brightness >= BREATH_MAX) {
    g_brightness = BREATH_MAX;
    g_velocity = -g_velocity;   // next steps fade out
  } else if (g_brightness <= BREATH_MIN) {
    g_brightness = BREATH_MIN;
    g_velocity = -g_velocity;   // next steps fade in
  }
}
