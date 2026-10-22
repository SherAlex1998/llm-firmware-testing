// stepper_28byj48: 28BYJ-48 stepper on a ULN2003 driver, full-step and half-step sequencing on 4 pins.
// Adapted from https://github.com/Moragor/Mora_28BYJ_48 (MIT); class converted to free functions.
#ifndef LOGIC_H
#define LOGIC_H
#include <Arduino.h>

#define STEPPER_PIN_IN1 8
#define STEPPER_PIN_IN2 9
#define STEPPER_PIN_IN3 10
#define STEPPER_PIN_IN4 11
#define STEPPER_SEQUENCE_LENGTH 8
#define STEPPER_DEFAULT_DELAY_MS 2

void stepper_init();                          // pins OUTPUT, coils off, phase 0, position 0
void stepper_set_delay(unsigned int ms);      // delay after each step
unsigned int stepper_delay();
uint8_t stepper_coil_pattern(uint8_t phase);  // pure: 4-bit coil pattern for phase 0..7 (bit0 = IN1); 0 if invalid
bool stepper_apply_phase(uint8_t phase);      // write the coil pattern of a phase to the pins; false if invalid
int8_t stepper_phase();                       // current phase index 0..7
long stepper_position();                      // signed position in half-steps (full step = 2)
void stepper_step_full(int steps);            // >0 forward, <0 backward; uses the two-coil phases (1,3,5,7)
void stepper_step_half(int steps);            // >0 forward, <0 backward; walks all 8 phases
void stepper_release();                       // all coils off (phase/position unchanged)

#endif
