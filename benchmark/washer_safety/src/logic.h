// Washing machine safety controller: fill -> wash -> spin -> done, door interlock, imbalance and fill-timeout alarms.
// Source: https://github.com/Lee-geonmu/arduino-washer-safety-system, MIT License, Copyright (c) 2026 Leegeonmu.
// Refactored for testability (see module.yaml notes).
#pragma once
#include <Arduino.h>

#define PIN_DOOR 2        // reed switch module, LOW = door closed
#define PIN_TILT 3        // tilt switch to GND (INPUT_PULLUP): LOW = still, HIGH = shaking
#define PIN_BUZZER 10     // passive buzzer
#define PIN_RELAY 11      // water valve relay
#define PIN_LED_LOCK 12   // red LED: door locked/closed
#define PIN_LED_OPEN 13   // green LED: door open
#define PIN_WATER A0      // water level sensor (analog)
#define PIN_BTN A1        // three buttons on one resistor ladder (analog)
// LCD 16x2 (parallel): RS 4, E 5, D4..D7 6..9. Stepper 28BYJ-48 via ULN2003: IN1..IN4 = A2..A5.

enum State {
  IDLE, FILLING, WASHING, SPINNING, DONE, ALARM, PAUSED, SLEEPING
};

#define LCD_INTERVAL 300UL            // ms between LCD refreshes
#define FILL_DURATION 20000UL         // fill timeout
#define WASH_DURATION 10000UL
#define SPIN_DURATION 8000UL
#define WATER_FULL_THRESHOLD 660      // ADC
#define BTN1_ADC 1019                 // button 1: start / acknowledge
#define BTN2_ADC 509                  // button 2: sleep on / off
#define BTN3_ADC 338                  // button 3: pause / resume
#define BTN_TOLERANCE 40
#define BTN_DEBOUNCE 250UL
#define DOOR_OPEN_CONFIRM_TIME 2000UL // door must stay open this long while running to pause
#define FILL_COMPLETE_HOLD 2000UL     // "100%" stays on the LCD before washing starts
#define CLOSE_DOOR_MSG_DURATION 2000UL
#define EEPROM_ADDR_STATE 0

void washer_init();                   // setup(): pins, LCD, stepper speed; restores the last state saved in EEPROM
void washer_loop();                   // one loop() pass reading millis() and the sensors
void washer_loop_with(uint32_t now_ms, int btn_adc, bool door_closed, bool tilt_high, int water_adc);  // the same pass with given time and sensor values
int washer_decode_button(int adc);    // button for an A1 reading: 0 none, 1, 2 or 3 (no debounce)
int washer_water_percent(int adc);    // water level in % from the A0 reading (piecewise linear)
bool washer_is_running_state(State s);  // FILLING, WASHING or SPINNING

State washer_state();
State washer_paused_from();           // state that was paused
bool washer_paused_by_door();
bool washer_paused_by_imbalance();
int washer_active_melody();           // -1 none, 0 start, 1 alarm (repeats), 2 complete
bool washer_melody_playing();
bool washer_lcd_on();                 // false in sleep mode
const char* washer_lcd_line(uint8_t row);  // text on LCD row 0 or 1: 16 characters, empty cells are spaces
