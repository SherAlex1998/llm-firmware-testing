// traffic_servo_gate: 3-LED traffic light whose green phase opens a servo barrier.
// Adapted from https://github.com/Rifat1003/Simple-Traffic-control-system-using-LED-and-servo-motor./tree/main (MIT).
#ifndef LOGIC_H
#define LOGIC_H
#include <Arduino.h>
#include <Servo.h>

#define TSG_PIN_GREEN  11
#define TSG_PIN_YELLOW 12
#define TSG_PIN_RED    13
#define TSG_PIN_SERVO  10
#define TSG_GATE_OPEN_ANGLE   180
#define TSG_GATE_CLOSED_ANGLE 0

enum TsgState : uint8_t {
  TSG_GREEN = 0,    // green LED, gate open
  TSG_YELLOW = 1,   // yellow LED, gate closed
  TSG_RED = 2,      // red LED, gate closed
  TSG_OFF = 3,      // all LEDs off, gate unchanged
  TSG_STATE_COUNT = 4
};

void tsg_init();                                    // pins configured, servo attached, enter TSG_GREEN
bool tsg_set_state(uint8_t state);                  // drive LEDs and servo for a state; false if invalid
uint8_t tsg_state();
uint8_t tsg_next_state(uint8_t state);              // pure: cyclic successor
void tsg_next();                                    // set_state(next_state(current))
int tsg_gate_angle();                               // Servo::read()
bool tsg_gate_open();                               // gate angle >= 90
unsigned int tsg_state_duration_ms(uint8_t state);  // pure: 5000 for every valid state

#endif
