// led_breathing: PWM fade-in / fade-out ("breathing") of one LED.
// Adapted from https://github.com/italianproject423/Fade-In-Fade-Out (MIT).
#ifndef LOGIC_H
#define LOGIC_H
#include <Arduino.h>

#define BREATH_PIN 11               // PWM pin
#define BREATH_MIN 0
#define BREATH_MAX 255
#define BREATH_DEFAULT_VELOCITY 2

void breath_init();                     // pin as OUTPUT, brightness 0, velocity +2
void breath_reset();                    // brightness 0, velocity +2, output 0
void breath_set_velocity(int velocity); // magnitude/sign of each step (0 is ignored)
int breath_velocity();
int breath_brightness();                // current logical brightness 0..255
bool breath_is_rising();                // velocity > 0
void breath_step();                     // write current brightness, then advance and bounce at 0/255

#endif
