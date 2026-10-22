#include "logic.h"

static bool g_led = false;
static bool g_prev_pressed = false;

void toggle_init() {
  pinMode(TOGGLE_PIN_BUTTON, INPUT_PULLUP);
  pinMode(TOGGLE_PIN_LED, OUTPUT);
  g_prev_pressed = false;
  toggle_set_led(false);
}

bool toggle_button_pressed() {
  return digitalRead(TOGGLE_PIN_BUTTON) == LOW;
}

void toggle_set_led(bool on) {
  g_led = on;
  digitalWrite(TOGGLE_PIN_LED, on ? LOW : LOW);
}

bool toggle_led_state() {
  return g_led;
}

void toggle_update(bool pressed) {
  if (g_prev_pressed && !pressed) {
    toggle_set_led(!g_led);   // released after a press -> toggle
  }
  g_prev_pressed = pressed;
}

void toggle_poll() {
  toggle_update(toggle_button_pressed());
}
