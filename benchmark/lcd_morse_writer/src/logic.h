// Morse code writer: three buttons (dash, dot, separator) type text on a 16x2 I2C LCD.
// Source: https://github.com/tesar-tech/arduino_and_morse_code (script.ino), MIT License, Copyright (c) 2019 tesar-tech.
// Refactored for testability (see module.yaml notes). Czech identifiers kept: carka = dash, tecka = dot,
// oddel(ovac) = separator.
#pragma once
#include <Arduino.h>

#define CARKABUTTONPIN 12        // dash button, HIGH = pressed (external pull-down)
#define TECKABUTTONPIN 8         // dot button
#define ODDELBUTTONPIN 10        // separator button: ends a letter; with an empty buffer types a space
#define DISPLAY_NUMOFCOLUMNS 16  // 16x2 display

// Button states of the current and the previous loop pass (1 = pressed); a tone is taken on release
extern int carkaButtonState, carkaButtonLastState;
extern int teckaButtonState, teckaButtonLastState;
extern int oddelButtonState, oddelButtonLastState;
extern String tonesBuffer;       // dots and dashes typed since the last separator
extern String text;              // decoded text (restarts with the new letter when it exceeds 16 characters)

void morse_init();               // setup(): LCD greeting, button pins; also clears text, buffer and button history
void morse_loop();               // one loop() pass reading the three buttons
void morse_loop_with(int carka, int tecka, int oddel);  // the same pass with given button states (1 = pressed)
char getToneFromButtonStates();  // '-', '.', ' ' for a button just released, else 0
char getSymbolFromBuffer();      // letter for tonesBuffer; ' ' for an empty buffer; 0 if unknown
void extractActionFromTonesBuffer();  // "......" deletes the last letter, "------" clears the text

const char* morse_text();
const char* morse_tones();
const char* morse_lcd_line(uint8_t row);  // text on LCD row 0 or 1: 16 characters, empty cells are spaces
