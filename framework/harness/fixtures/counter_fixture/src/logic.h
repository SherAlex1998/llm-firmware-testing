#pragma once
#include <Arduino.h>

#define OVF_LED_PIN 13

void cnt_init();
void cnt_add(uint8_t n);        // counts modulo 100; lights the LED on overflow
uint8_t cnt_value();
uint8_t cnt_digit(uint8_t pos); // pos 0 = ones, 1 = tens (display.cpp)
