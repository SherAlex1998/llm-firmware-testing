// traffic_light: 3-LED traffic light state machine (red -> red+yellow -> green -> green blink -> yellow).
// Adapted from https://github.com/sspeev/Arduino-Traffic-Light (MIT).
#ifndef LOGIC_H
#define LOGIC_H
#include <Arduino.h>

#define TL_PIN_RED    2
#define TL_PIN_YELLOW 3
#define TL_PIN_GREEN  4

enum TlState : uint8_t {
  TL_RED = 0,
  TL_RED_YELLOW = 1,
  TL_GREEN = 2,
  TL_GREEN_BLINK = 3,
  TL_YELLOW = 4,
  TL_STATE_COUNT = 5
};

void tl_init();                                   // configure pins, enter TL_RED
void tl_set_state(uint8_t state);                 // drive the 3 LEDs for the given state (ignored if invalid)
uint8_t tl_state();                               // current state
uint8_t tl_next_state(uint8_t state);             // pure: next state in the cycle
void tl_advance();                                // set_state(next_state(current))
unsigned int tl_state_duration_ms(uint8_t state); // pure: how long each state lasts
void tl_blink_green(uint8_t times, unsigned int half_period_ms); // blink green LED, ends with green LOW

#endif
