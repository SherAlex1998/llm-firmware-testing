// Tabletop RPG dice roller: a potentiometer selects the die (d2..d100), a movement detector rolls it, the 16x2
// LCD shows the result in decimal and in base-5 'Murasaki' digits (custom characters) plus a seconds counter.
// Source: https://github.com/rchampeimont/arduino_JDR_Murasaki (JDR_Murasaki/JDR_Murasaki.ino), MIT License,
// (c) rchampeimont. Refactored for testability (see module.yaml notes).
#pragma once
#include <Arduino.h>

extern const int SELECTOR_PIN;
extern const int MOVE_DETECT_PIN;
extern const int LCD_RS_PIN;
extern const int LCD_ENABLE_PIN;
extern const int LCD_DATA_PIN_0;
extern const int LCD_DATA_PIN_1;
extern const int LCD_DATA_PIN_2;
extern const int LCD_DATA_PIN_3;
extern const int LCD_ROWS;
extern const int LCD_COLS;
extern const int N_DICES;
extern const int dices[];             // { 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 20, 22, 24, 30, 100 }
extern int selectedDiceIndex;         // -1 before the first selection
extern int selectedDiceType;          // sides of the selected die
extern int prevSwitchState;           // last level of the movement detector
extern unsigned long secondsSinceStart;
extern int lastRoll;                  // result of the last roll (testability)

void jdr_set_delay_scale(uint8_t percent);  // roll animation (0.8 s) and result pause (0.5 s): 100 = original (default), 0 = none
void jdr_reset_state();               // globals back to power-up values (jdr_init calls it)
void jdr_init();                      // setup(): pins, LCD, Murasaki digit characters
void jdr_loop();                      // loop(): selectDice(), rollDice(), updateMurasakiClock()
void selectDice();                    // reads the potentiometer (A0)
void selectDiceWith(int selectorRawValue);   // the same for a given ADC value: map(raw, 1000, 20, 0, 14)
void rollDice();                      // reads the movement detector (pin 6); a falling edge rolls
void rollDiceWith(int switchState, unsigned long now);  // the same with a given level and time (random seed)
void updateMurasakiClock();
void updateMurasakiClockAt(unsigned long now);
void printMurasakiDigit(int value);
void printMurasakiNumber(unsigned long value, int littleDigitCol, int row);  // base 5; littleDigitCol -1 = at the cursor
void printDiceType(int col, int row);
const char *jdr_lcd_line(uint8_t row);  // LCD row 0/1: 16 chars, Murasaki digits as '0'..'4', full block as '#'
