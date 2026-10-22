// servo_sweep: sweep a hobby servo back and forth between two angles with configurable step and timing.
// Adapted from https://github.com/gwappa/arduino-servo-sweep (MIT).
#ifndef LOGIC_H
#define LOGIC_H
#include <Arduino.h>
#include <Servo.h>

#define SWEEP_SERVO_PIN 9
#define SWEEP_START 130   // default lower angle
#define SWEEP_STOP  142   // default upper angle

void sweep_init();                                     // attach the servo and move to SWEEP_START
int sweep_clamp_angle(int angle);                      // pure: clamp to 0..180
void sweep_write(int angle);                           // clamp and write the angle to the servo
int sweep_angle();                                     // angle currently commanded (Servo::read)
unsigned int sweep_steps_per_cycle(int start_pos, int stop_pos, int step_pos); // pure: number of writes per up+down cycle (0 if args invalid)
void sweep_run(int start_pos, int stop_pos, int step_pos, int cycles, int step_delay_ms); // run cycles of up-and-down sweeps
unsigned long sweep_write_count();                     // number of servo writes since sweep_init

#endif
