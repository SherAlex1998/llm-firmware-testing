#ifndef Pace_h
#define Pace_h
#include <Arduino.h>

// Testability: one scale for every blocking pause of the module (melodies, text scrolling, statistics pages).
extern uint8_t g_delay_scale;   // percent, 100 = original timing

inline unsigned long paced(unsigned long ms) {
  return ms * g_delay_scale / 100;
}

inline void paced_delay(unsigned long ms) {
  delay(paced(ms));
}

#endif
