// dc_motor_pot: DC motor via L298N; speed from a potentiometer (PWM on EN), direction on IN1/IN2.
// Adapted from https://github.com/AndreaLombardo/Arduino-L298N-Motore-DC (MIT).
#ifndef LOGIC_H
#define LOGIC_H
#include <Arduino.h>

#define MOTOR_PIN_POT A0    // potentiometer (analog)
#define MOTOR_PIN_IN1 12    // L298N IN1
#define MOTOR_PIN_IN2 13    // L298N IN2
#define MOTOR_PIN_EN  11    // L298N EN (PWM)

void motor_init();                          // pins configured, forward direction, power 0
void motor_set_direction(bool forward);     // forward: IN1 LOW / IN2 HIGH; reverse: IN1 HIGH / IN2 LOW
bool motor_direction();                     // true = forward
void motor_set_power(int pwm);              // clamped to 0..255, written to EN
int motor_power();                          // last PWM value written
int motor_power_from_reading(int raw);      // pure: ADC 0..1023 -> PWM 0..255
void motor_stop();                          // IN1 LOW, IN2 LOW, power 0
void motor_update();                        // read potentiometer and apply the power

#endif
