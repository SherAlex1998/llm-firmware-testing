// Wake-up light: alarm clock that fades an LED strip in over 30 minutes, IR remote control, 16x2 I2C LCD, RTC.
// Source: https://github.com/erhanalankus/Arduino-Wake-Up-Light (Sketch.ino), MIT License, (c) erhanalankus.
// Refactored for testability (see module.yaml notes).
#pragma once
#include <Arduino.h>

extern int RECV_PIN;                   // 14 (A0): IR receiver
#define backlightLCD 9                 // LCD backlight (PWM)
#define WhiteLEDStrip 5                // alarm light (PWM)
#define RedLEDStrip 6                  // soft evening/night light (PWM)
#define ticker 2                       // "tick" buzzer
#define DS3231_I2C_ADDRESS 0x68        // RTC (time registers 0..6, BCD)

extern int activeLED;                  // WhiteLEDStrip or RedLEDStrip
extern int brightness, brightnessStep, smallBrightnessStep;
extern bool lightOn;
extern byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;   // last RTC reading
extern byte alarmHour, alarmMinute;    // default 06:30
extern bool alarmOn, alarmActive;
extern long alarmTurnedOnAt, alarmStepTime, lastTimeReading, alarmDuration;
extern long startTickingAfter, startingTickerInterval, tickerInterval, tickerStateChangeTime;
extern long tickerStateChangedAt, tickerToggledAt;
extern bool tickerActive, tickerOn, tickerEnabled;
extern int backlightBrightnessForNight, backlightBrightnessForDay;
extern bool isNight, backlightManuallyTurnedOff, nightLightEnabled;

void wake_reset_state();               // globals with initialisers back to their power-up values (wake_init calls it)
void wake_init();                      // setup(): I2C, LCD and icons, IR receiver, pins, first RTC reading
void wake_loop();                      // one loop() pass: millis() and the IR receiver
void wake_loop_with(unsigned long now_ms, unsigned long ir_code);  // the same pass with a given time and IR code (0 = none)
void HandleIRCode(unsigned long code); // NEC codes of the author's remote, e.g. 0xFFA25D PLAY = light on, 0xFF22DD EQ = off
byte decToBcd(byte val);
byte bcdToDec(byte val);
void ActivateNightLight();
void DecreaseTickerInterval();
void ToggleTicker();
void LightOn();
void LightOff();
void SetBrightness(int brightnessValue);
void PushedAnyButton();
void ToggleActiveLEDStrip();
bool OnceEverySecond();
void ReadTime();
void CheckAlarm();
void Alarm();
void RefreshLCD();
String DayOfWeekForLCD();
void PrintTimeValueOnLCD(byte value);
void ChangeBrightness(int step);
void IncreaseAlarmHour();
void IncreaseAlarmMinute();
void ToggleAlarm();
void CheckDayOrNight();
void SetLCDBacklight();
void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year);
void readDS3231time(byte *second, byte *minute, byte *hour, byte *dayOfWeek, byte *dayOfMonth, byte *month, byte *year);
