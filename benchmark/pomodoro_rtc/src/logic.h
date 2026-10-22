// Pomodoro timer: TM1637 4-digit display, DS1307 RTC (statistics kept in its RAM), one button, buzzer.
// Source: https://github.com/2CoderOK/pomodoro (src/), MIT License, Copyright (c) 2022 CoderOK.
// Refactored for testability (see module.yaml notes). Internal files: state_machine, click, display4, rtsl, soundbuzz.
#pragma once
#include <Arduino.h>

#define TM_CLK_PIN 3      // TM1637 display
#define TM_DIO_PIN 4
#define BTN_CLICK 6       // button, HIGH = pressed (external pull-down)
#define BTN_BUZZER 9      // passive buzzer

#define STATUS_IDLE 0
#define STATUS_PREPARE 1
#define STATUS_RUN_WORK 2
#define STATUS_RUN_PAUSE 3
#define STATUS_RUN_REST 4

#define DEFAULT_TIME 25   // minutes
#define INCREASE_TIME 5
#define MAX_TIME 60

#define CLICK_NONE 0
#define CLICK_SHORT_ACTION 1
#define CLICK_LONG_ACTION 2

void pomo_set_delay_scale(uint8_t percent);  // pauses of melodies, text scrolling and statistics pages: 100 = original (default), 0 = none; kept across pomo_init()
void pomo_init();                  // state machine back to power-up values, then the original Init(): RTC, statistics, display, start melody, idle
void pomo_start_timer();           // Timer1 every 500 ms calls pomo_tick() (the sketch does this after pomo_init)
void pomo_tick();                  // half-second tick (interrupt body): counts seconds down, sets the seconds event
void pomo_update();                // one pass of loop(): handles a pending seconds event, else the button and the display
void pomo_handle_action(uint8_t action);  // what the state machine does on a click: CLICK_SHORT_ACTION or CLICK_LONG_ACTION
uint8_t pomo_read_click();         // button pass of loop() (Click::GetClick): returns the action of a press that just ended

uint8_t pomo_status();
uint8_t pomo_prev_status();
int8_t pomo_minute();              // minutes shown (pomodoro_minute)
int8_t pomo_second();
int8_t pomo_completed();           // pomodoros completed today
bool pomo_seconds_event();
uint16_t pomo_user_data(uint8_t index);  // statistics: 0 day of month, 1 today, 2 this week, 3 this month, 4 total
void pomo_load_user_data();        // reads statistics from the RTC RAM and rolls them over to the RTC date
void pomo_apply_rollover(uint16_t data[5], uint16_t year, uint8_t month, uint8_t day, uint8_t day_of_week);  // rollover rules on an explicit array and date (Sunday = 0)
uint8_t pomo_calc_weekday(int year, int month, int day);  // proleptic Gregorian, Sunday = 0
const int8_t* pomo_display_digits();     // last 4 cells sent to the display (0-9, 0x40 = dash, '-' = progress mark)
bool pomo_display_dot();                 // colon state
const char* pomo_display_text();         // last text shown with DisplayString (empty if none since init)
