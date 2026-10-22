// railway_gate: two servo-driven gates plus red/green LEDs, controlled by two digital (IR) sensors.
// Adapted from https://github.com/Venkatesh4707/Railway-Gate-Automation-Arduino (MIT).
#ifndef LOGIC_H
#define LOGIC_H
#include <Arduino.h>
#include <Servo.h>

#define GATE_PIN_SENSOR1   A0   // digital input, LOW = object detected
#define GATE_PIN_SENSOR2   A1   // digital input, LOW = object detected
#define GATE_PIN_LED_GREEN 2
#define GATE_PIN_LED_RED   4
#define GATE_PIN_SERVO1    11
#define GATE_PIN_SERVO2    10
#define GATE_SERVO1_CLOSED 180
#define GATE_SERVO1_OPEN   90
#define GATE_SERVO2_CLOSED 90
#define GATE_SERVO2_OPEN   180
#define GATE_SWEEP_STEP_DEG 5
#define GATE_SWEEP_DELAY_MS 10

void gate_init();                                          // attach servos, gates closed, red LED on
void gate_open();                                          // sweep both servos to the open angles; green on, red off
void gate_close();                                         // sweep both servos to the closed angles; red on, green off
bool gate_is_open();
bool gate_object_between();                                // an object entered via sensor 1 and has not left via sensor 2
int gate_servo1_angle();                                   // Servo::read()
int gate_servo2_angle();
bool gate_sensor1_detected();                              // digitalRead(GATE_PIN_SENSOR1) == LOW
bool gate_sensor2_detected();
void gate_update(bool sensor1_detected, bool sensor2_detected); // state machine step with explicit sensor values
void gate_poll();                                          // read sensors and call gate_update

#endif
