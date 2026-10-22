// button_toggle_led: a push-button (INPUT_PULLUP, active LOW) toggles an LED on each press/release.
// Adapted from https://github.com/harsh-hw-dev/Embedded-Programming-Series ("LED WITH BUTTON", MIT).
#ifndef LOGIC_H
#define LOGIC_H
#include <Arduino.h>

#define TOGGLE_PIN_BUTTON 2   // INPUT_PULLUP: LOW = pressed
#define TOGGLE_PIN_LED    13

void toggle_init();                 // pins configured, LED off, no press remembered
bool toggle_button_pressed();       // digitalRead(TOGGLE_PIN_BUTTON) == LOW
void toggle_update(bool pressed);   // feed the current button level; LED toggles on the release edge
void toggle_poll();                 // toggle_update(toggle_button_pressed())
bool toggle_led_state();            // true = LED on
void toggle_set_led(bool on);       // force LED state

#endif
