// Pomodoro timer on a 24-LED NeoPixel ring with one button (long press: on/off, short press: +1 minute).
// Source: https://github.com/atbrask/NeoPomodorino (Firmware/NeoPomodorino), BSD 2-Clause License,
// Copyright (c) 2017, Asbjorn Brask. Refactored for testability (see module.yaml notes).
#pragma once
#include <Arduino.h>

#define BUTTON_PIN     3      // to GND, INPUT_PULLUP: LOW = pressed
#define PIXEL_PIN      6
#define PIXEL_COUNT    24

#define POMODORO_LIMIT 4
#define POMODORO_TIME  25     // minutes
#define SHORT_PAUSE    5
#define LONG_PAUSE     25
#define LUMINANCE      16

enum State
{
  Off = 0,
  Pomodoro = 1,
  ShortPause = 2,
  LongPause = 3
};

enum ButtonClick
{
  None,
  ShortClick,
  LongClick
};

extern State currentState;
extern int pomodoro;              // pomodoro number 1..4 (0 when off)
extern unsigned long starttime;   // ms
extern unsigned long duration;    // ms

void neo_set_delay_scale(uint8_t percent);  // blocking LED animations (blink 1.2 s, fill 0.5 s): 100 = original (default), 0 = none
void neo_init();                  // setup(): button pin, ring, off
void neo_loop();                  // one loop() pass with millis() and the button
void neo_loop_at(unsigned long now, ButtonClick button);  // the same pass with a given time and click
ButtonClick getButtonClick();     // reads the button: a press up to ~0.5 s is short, longer is long
void turnOff();
void startPomodoro();
void startPause();
void updateDisplay();             // ring colours from the elapsed fraction (red work, yellow short pause, green long)
uint32_t neo_pixel(uint8_t i);    // colour of ring pixel i (0x00RRGGBB)
