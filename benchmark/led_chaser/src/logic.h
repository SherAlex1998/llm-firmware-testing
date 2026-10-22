// led_chaser: 10 LEDs on D2..D11 lit one at a time, bouncing back and forth.
// Adapted from https://github.com/Projectslearner/arduino-uno-led-chaser (MIT).
#ifndef LOGIC_H
#define LOGIC_H
#include <Arduino.h>

#define CHASER_NUM_LEDS 10
extern const uint8_t CHASER_PINS[CHASER_NUM_LEDS]; // {2,3,4,5,6,7,8,9,10,11}

void chaser_init();                       // pins as OUTPUT, all off, direction forward
void chaser_all_off();                    // all LEDs LOW, position becomes -1
bool chaser_light_only(uint8_t index);    // exactly one LED on; false if index out of range
int8_t chaser_position();                 // index of the lit LED, -1 if none
int8_t chaser_direction();                // +1 forward (towards D11), -1 backward
void chaser_step();                       // move one LED in current direction, bounce at the ends
uint8_t chaser_count_on();                // number of LED pins currently reading HIGH

#endif
