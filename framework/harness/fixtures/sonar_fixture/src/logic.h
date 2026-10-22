#pragma once
#include <Arduino.h>

#define TRIG_PIN 9
#define ECHO_PIN 10

void sonar_init();
uint16_t sonar_read_cm();  // 0 when no echo within 30 ms
