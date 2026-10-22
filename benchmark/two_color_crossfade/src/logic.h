// two_color_crossfade: red/green 2-color LED module cross-faded with PWM.
// Adapted from https://github.com/Projectslearner/Arduino-uno-2-color-LED-module (MIT).
#ifndef LOGIC_H
#define LOGIC_H
#include <Arduino.h>

#define XF_PIN_RED   11   // PWM
#define XF_PIN_GREEN 10   // PWM
#define XF_MAX_LEVEL 150  // maximum PWM level used by the fade

void xf_init();             // pins OUTPUT, phase 0, value 0
void xf_apply(int value);   // value clamped to 0..XF_MAX_LEVEL; phase 0: red=MAX-value, green=value; phase 1: swapped
void xf_step();             // value+1; after XF_MAX_LEVEL wrap to 0 and switch phase
int xf_value();             // current fade value 0..XF_MAX_LEVEL
uint8_t xf_phase();         // 0 = red fades out / green fades in, 1 = the opposite
int xf_red_level();         // PWM level last written to the red channel
int xf_green_level();       // PWM level last written to the green channel

#endif
