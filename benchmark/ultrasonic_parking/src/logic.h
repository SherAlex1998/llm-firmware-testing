// Garage parking assistant: HC-SR04 distance -> colour of one NeoPixel; a button stores the current distance as target.
// Source: https://github.com/acheaito/arduino-garage-parking, MIT License, Copyright (c) 2016 Ali Cheaito.
// Refactored for testability (see module.yaml notes). Classes: sonar_controller.h, led_controller.h,
// button_controller.h (included as .cpp files by the original sketch).
#pragma once
#include <Arduino.h>
#include "led_controller.h"
#include "sonar_controller.h"
#include "button_controller.h"

//I/O pins (LED and button moved off pins 0/1, which carry Serial on the Uno)
#define TRIGGER_PIN 4
#define ECHO_PIN 3
#define LED_PIN 6
#define SETUP_BUTTON_PIN 2

extern SonarController sonar;          // zones relative to the target distance (default 30 cm), idle timeout 30 s
extern LedController led;              // purple: approaching, green: in zone, yellow: past zone, flashing red: too close
extern ButtonController setupButton;   // released() of the button (INPUT_PULLUP, LOW = pressed)

void park_init();                      // setup(): LED purple; also resets the sonar state (target, readings, idle timer)
void park_loop();                      // one loop() pass: sensor reading, idle check, LED colour, setup button
void park_loop_with(int distance, bool button_released, unsigned long now_ms);  // the same pass with a given reading, button and time
void setLedColor();                    // LED colour for the current zone
