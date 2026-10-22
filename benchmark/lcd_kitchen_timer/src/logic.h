// Kitchen timer: 8x2 LCD, two buttons, buzzer, battery gauge; the alarm time is kept in EEPROM.
// Source: https://github.com/SlimTim10/kitchen-timer (kitchen-timer/kitchen-timer.ino), MIT License,
// Copyright (c) 2016 SlimTim10. Refactored for testability (see module.yaml notes).
#pragma once
#include <Arduino.h>

enum pins {
	LCD_RS = 7,
	LCD_E = 8,
	LCD_DB4 = 9,
	LCD_DB5 = 10,
	LCD_DB6 = 11,
	LCD_DB7 = 12,
	BUTTON1 = 3,
	BUTTON2 = 2,
	BUZZER = 5,
	BATTERY = 0,
};

enum config {
	TONE_FREQ = 2110,
	BEEPS = 4,
	BEEP_DELAY = 100,
	BEEP_GROUPS = 3,
	BEEP_GROUP_DELAY = 1000,
};

enum constants {
	DEBOUNCE_MS = 50,
	MINUTES_ADDRESS = 0,
	SECONDS_ADDRESS = 0 + sizeof(int),
};

enum battery_icons {
	BAT_FULL_ICON,
	BAT_HIGH_ICON,
	BAT_LOW_ICON,
	BAT_EMPTY_ICON,
};

enum battery_icon_pos {
	BAT_ICON_COL = 7,
	BAT_ICON_ROW = 0,
};

/* Voltage * 100 to deal with int instead of float */
enum battery_voltage {
	BAT_FULL_VOLT = 380,
	BAT_HIGH_VOLT = 370,
	BAT_LOW_VOLT = 360,
	BAT_EMPTY_VOLT = 350,
};

enum State {
	IDLE = 0,
	RUNNING,
};

extern State state;
extern int alarm_minutes, alarm_seconds;   // alarm time (EEPROM addresses MINUTES_ADDRESS, SECONDS_ADDRESS)
extern int minutes, seconds;               // remaining time while RUNNING
extern boolean first_press;                // first BUTTON2 press clears the alarm time
extern char time_string[6];

void ktimer_set_delay_scale(uint8_t percent);  // 1 s count-down step, 0.1 s key repeat and ~5.4 s alarm: 100 = original (default), 0 = none
void ktimer_init();                // setup(): pins, LCD, icons, alarm time from EEPROM, IDLE
void ktimer_loop();                // one loop() pass (buttons read with button_press)
void ktimer_loop_with(boolean button1, boolean button2);  // the same pass with given debounced presses
void count_down(void);
void display_alarm_time(void);
void alarm(void);
void increase_alarm_time(void);
boolean button_press(uint8_t pin);  // HIGH = pressed (external pull-down), 50 ms debounce
int battery_voltage(void);          // volts * 100 from A0
void display_battery(void);
const char* ktimer_lcd_line(uint8_t row);  // LCD row 0/1: 8 characters, battery icons shown as '0' full .. '3' empty
