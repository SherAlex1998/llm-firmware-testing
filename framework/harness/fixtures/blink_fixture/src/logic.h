// blink_fixture: minimal module used by the harness self-tests (LED on pin 13).
// Based on the Arduino built-in "Blink" example (CC0-1.0 / public domain).
#ifndef LOGIC_H
#define LOGIC_H
#include <Arduino.h>

#define LED_PIN 13

void led_init();                                        // pin as OUTPUT, LED off
void led_on();
void led_off();
void led_toggle();
int led_state();                                        // digitalRead(LED_PIN): HIGH or LOW
void led_blink(uint8_t times, unsigned int half_period_ms); // times x (on, off); ends with LED off
unsigned int led_on_count();                            // number of times the LED was switched on

#endif
