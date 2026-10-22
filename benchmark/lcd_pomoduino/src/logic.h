// Pomodoro timer with a 16x2 LCD, two buttons, a beeper and the pomodoro count kept in EEPROM.
// Source: https://github.com/opokatech/pomoduino2 (src/ + src-alternative/ui/lcd), MIT License,
// Copyright (c) 2016 Miroslaw Opoka. Refactored for testability (see module.yaml notes).
//
// Classes (namespace MO): Pomodoro (state machine), Button / Button_Logic (press, release, durations), Timer,
// Period, Counter, Beeper, Display, UI (LCD user interface), Eeprom. Pins and periods: Const.hpp.
// Buttons: B1 short release cycles the period type (25 / 5 / 15 min); B2 short click (< 250 ms) starts, B2
// release while running pauses; holding B2 over 3 s in READY clears the pomodoro count.
#pragma once
#include <Arduino.h>
#include "Const.hpp"
#include "Pomodoro.hpp"
#include "Button.hpp"
#include "Eeprom.hpp"
#include "UI.hpp"

extern MO::Button   the_b1;        // pin 3, INPUT_PULLUP, LOW = pressed
extern MO::Button   the_b2;        // pin 4, INPUT_PULLUP, LOW = pressed
extern MO::Eeprom   the_eeprom;
extern MO::UI       the_ui;
extern MO::Pomodoro the_pomodoro;

#define POM_READY 0
#define POM_RUNNING 1
#define POM_FINISHED 2
#define POM_PAUSED 3

void pomo2_setup();                 // setup(): buttons, UI; pomodoro object re-created (count read from EEPROM)
void pomo2_loop_at(uint32_t a_time);  // one loop() pass at time a_time (ms): buttons read from the pins, Run()
void pomo2_step(uint32_t a_time, bool b1_down, bool b2_down);  // the same pass with given button states
const char *pomo2_lcd_line(uint8_t a_y);  // last text written to LCD line 0 or 1
