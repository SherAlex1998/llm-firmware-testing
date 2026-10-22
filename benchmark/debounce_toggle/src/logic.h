// debounce_toggle: LED toggles on each debounced LOW->HIGH transition of a push-button.
// Adapted from the Arduino built-in example "Debounce"
// https://github.com/arduino/arduino-examples/blob/main/examples/02.Digital/Debounce/Debounce.ino (CC0-1.0 / public domain).
#ifndef LOGIC_H
#define LOGIC_H
#include <Arduino.h>

#define DB_PIN_BUTTON 2               // INPUT with external pull-down: HIGH = pressed
#define DB_PIN_LED    13
#define DB_DEFAULT_DEBOUNCE_MS 50

void db_init();                                     // pins configured, LED starts HIGH (as in the original)
void db_update(int reading, unsigned long now_ms);  // core logic with an explicit time source
void db_poll();                                     // db_update(digitalRead(button), millis())
int db_led_state();                                 // HIGH / LOW currently driven on the LED
int db_button_state();                              // last debounced button level
unsigned long db_debounce_delay();                  // ms
void db_set_debounce_delay(unsigned long ms);
unsigned long db_last_change_ms();                  // time of the last raw reading change

#endif
