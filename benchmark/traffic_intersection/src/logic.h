// traffic_intersection: 4-way intersection, 12 LEDs (red/green/yellow per direction), 8 phases.
// Adapted from https://github.com/rajeshThakare/Arduino-UNO-for-Absolute-Beginners (Codes/1_LED/traffic signals, MIT).
#ifndef LOGIC_H
#define LOGIC_H
#include <Arduino.h>

#define TI_NUM_DIRECTIONS 4
#define TI_NUM_PHASES 8

enum TiColor : uint8_t { TI_RED = 0, TI_GREEN = 1, TI_YELLOW = 2 };

extern const uint8_t TI_PIN_RED[TI_NUM_DIRECTIONS];     // {13, 10, 7, 4}
extern const uint8_t TI_PIN_GREEN[TI_NUM_DIRECTIONS];   // {12, 9, 6, 3}
extern const uint8_t TI_PIN_YELLOW[TI_NUM_DIRECTIONS];  // {11, 8, 5, 2}

void ti_init();                                        // pins OUTPUT; start-up state: all directions yellow
bool ti_set_direction(uint8_t dir, uint8_t color);     // drive the 3 LEDs of one direction; false if args invalid
uint8_t ti_direction_color(uint8_t dir);               // last color set for a direction (TI_RED if dir invalid)
bool ti_apply_phase(uint8_t phase);                    // phase p: direction p/2 gets GREEN (p even) or YELLOW (p odd), others RED
void ti_next_phase();                                  // apply (phase + 1) % TI_NUM_PHASES
uint8_t ti_phase();                                    // current phase index
bool ti_is_safe();                                     // true if at most one direction is not red

#endif
