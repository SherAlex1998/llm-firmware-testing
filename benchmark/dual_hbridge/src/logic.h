// dual_hbridge: two DC motors on an L298N/L293D H-bridge (IN1..IN4 direction, ENA/ENB PWM speed).
// Adapted from https://github.com/Projectslearner/arduino-uno-h-bridge-module (MIT).
#ifndef LOGIC_H
#define LOGIC_H
#include <Arduino.h>

#define HB_MOTOR_A 0
#define HB_MOTOR_B 1

#define HB_PIN_IN1 8    // motor A direction
#define HB_PIN_IN2 9
#define HB_PIN_ENA 6    // motor A speed (PWM)
#define HB_PIN_IN3 10   // motor B direction
#define HB_PIN_IN4 11
#define HB_PIN_ENB 5    // motor B speed (PWM)

void hb_init();                                   // pins OUTPUT, both motors stopped
bool hb_forward(uint8_t motor, uint8_t speed);    // INx HIGH / INy LOW, EN = speed; false if motor invalid
bool hb_backward(uint8_t motor, uint8_t speed);   // INx LOW / INy HIGH, EN = speed
bool hb_stop(uint8_t motor);                      // both IN LOW, EN 0
void hb_stop_all();
int8_t hb_direction(uint8_t motor);               // +1 forward, -1 backward, 0 stopped (or invalid motor)
uint8_t hb_speed(uint8_t motor);                  // last PWM speed set (0 if stopped)

#endif
