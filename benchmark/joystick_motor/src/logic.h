// joystick_motor: analog joystick axis -> DC motor speed and direction through an L293D.
// Adapted from https://github.com/vchervanev/arduino-joystick-to-motor (MIT).
#ifndef LOGIC_H
#define LOGIC_H
#include <Arduino.h>

#define JM_PIN_AXIS   A0   // joystick X axis (analog)
#define JM_PIN_SPEED  5    // L293D enable (PWM)
#define JM_PIN_DIR_A  3    // L293D input A
#define JM_PIN_DIR_B  4    // L293D input B
#define JM_AXIS_CENTER 512
#define JM_DEAD_ZONE   10  // speeds below this stop the motor

void jm_init();                          // pins configured, clockwise, stopped
void jm_set_direction(bool clockwise);   // clockwise: DIR_A HIGH / DIR_B LOW; otherwise swapped
bool jm_direction();                     // true = clockwise
void jm_set_speed(int speed);            // clamped to 0..255, written to the PWM pin
int jm_speed();                          // last speed written
void jm_stop();                          // speed 0
int jm_speed_from_axis(int dx);          // pure: |dx - 512| / 2
bool jm_clockwise_from_axis(int dx);     // pure: dx > 512
void jm_apply_axis(int dx);              // dead zone -> stop, else set direction and speed
void jm_update();                        // read the joystick and apply

#endif
