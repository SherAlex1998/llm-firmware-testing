// Pet feeding timer: one button records a feeding (repeated presses move it back an hour), LEDs show how long
// ago (green < 6 h, yellow < 9 h, red), a 16x2 LCD prints it, a second button toggles the backlight.
// Source: https://github.com/hlgirard/PetFeedingTimer (CatFeedingTime_Nano), MIT License, (c) hlgirard.
// Refactored for testability (see module.yaml notes).
#pragma once
#include <Arduino.h>

#define buttonPin 2
#define screenOnButtonPin 3
#define redLEDPin A2
#define yellowLEDPin A1
#define greenLEDPin A0
#define screenOnPin 10

extern const unsigned long hour;   // 3600000 milliseconds in an hour
extern const unsigned long minute;   // 60000 milliseconds in a minute
extern const unsigned long interval1;   // maybe a treat after 6 hours (yellow LED)
extern const unsigned long interval2;   // should feed again after 9 hours (red LED)
extern const unsigned long screenDelay;   // Turn screen Off after 7s delay
extern const unsigned long redrawInterval;   //Redraw screen every 0.8 seconds

extern volatile unsigned long lastFeedingTime;   // millis() of the last feeding
extern volatile unsigned long screenOnTime;
extern int screenState;
extern volatile unsigned long previousButtonTime, buttonTime;
extern volatile unsigned long previousScreenOnButtonTime, screenOnButtonTime;
extern volatile unsigned long previousRedrawTime;

void pet_reset_state();             // globals back to power-up values (pet_init calls it)
void pet_init();                    // setup(): pins, button interrupts (2, 3 on FALLING), LCD greeting
void pet_loop();                    // one loop() pass at millis()
void pet_loop_at(unsigned long currentTime);  // the same pass at a given time
void resetTime();                   // interrupt of the feeding button (uses millis())
void resetTimeAt(unsigned long now);  // the same at a given time
void screenON();                    // interrupt of the screen button
void screenONAt(unsigned long now);
void displayFeedingInfo(unsigned long deltaTime, unsigned long currentTime);
void ledFeedingStatus(unsigned long deltaTime);
const char *pet_lcd_line(uint8_t row);  // LCD row 0/1, 16 characters
