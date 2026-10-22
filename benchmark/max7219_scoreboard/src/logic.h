// Two-player scoreboard on four MAX7219 8x8 matrices: a short press adds a point, a press longer than 0.5 s resets
// that side. Source: https://github.com/TrevisanGMW/arduino-scoreboard (Score_Keeper_01.ino), MIT License,
// (c) TrevisanGMW. Refactored for testability (see module.yaml notes).
#pragma once
#include <Arduino.h>

#define MAX_DEVICES 4
#define CLK_PIN   13  // or SCK (hardware SPI)
#define DATA_PIN  11  // or MOSI
#define CS_PIN    10  // or SS
#define BUTTON_LEFT_PIN    4 // Data Pin for Left Button (HIGH = pressed)
#define BUTTON_RIGHT_PIN    2 // Data Pin for Right Button (HIGH = pressed)
#define CHAR_SPACING  1 // pixels between characters
#define BUF_SIZE  75

extern long buttonTimerR, longPressTimeR;     // long press threshold 500 ms
extern boolean buttonActiveR, longPressActiveR;
extern long buttonTimerL, longPressTimeL;
extern boolean buttonActiveL, longPressActiveL;
extern char message[BUF_SIZE];                 // text sent to the matrices, e.g. "3       12"
extern bool newMessageAvailable;
extern int currentScoreLeft, currentScoreRight;
extern String howMuchSpace;

void score_reset_state();                      // scores and button state back to power-up values (score_init calls it)
void score_init();                             // setup(): display, button pins
void score_loop();                             // one loop() pass reading the buttons and millis()
void score_loop_with(int buttonStateLeft, int buttonStateRight, unsigned long now);  // the same pass with given inputs
void printText(uint8_t modStart, uint8_t modEnd, char *pMsg);  // prints text right-aligned over the modules
uint8_t score_column(uint8_t col);             // pixel column 0..31 last written to the matrices (MD_MAX72XX::getColumn)
