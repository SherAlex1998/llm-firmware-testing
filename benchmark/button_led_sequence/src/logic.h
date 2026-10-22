// button_led_sequence: each button press advances a 4-LED sequence (only one LED lit at a time).
// Adapted from https://github.com/rajeshThakare/Arduino-UNO-for-Absolute-Beginners (Codes/3_Switch/switch_practical_3, MIT).
#ifndef LOGIC_H
#define LOGIC_H
#include <Arduino.h>

#define SEQ_PIN_BUTTON 2      // INPUT with external pull-down: HIGH = pressed
#define SEQ_NUM_LEDS 4
extern const uint8_t SEQ_PINS[SEQ_NUM_LEDS];   // {7, 6, 5, 4}

void seq_init();                  // pins configured, all LEDs off, count 0
void seq_all_off();               // all LEDs LOW (count unchanged, current LED -1)
bool seq_apply(uint8_t step);     // LED[step] on and the previous LED off; false if step out of range
void seq_advance();               // apply current count, then count = (count + 1) % 4
uint8_t seq_count();              // index of the step that the next advance will apply (0..3)
int8_t seq_current_led();         // index of the LED currently lit, -1 if none
bool seq_button_pressed();        // digitalRead(SEQ_PIN_BUTTON) == HIGH
void seq_poll();                  // advance once if the button is pressed

#endif
