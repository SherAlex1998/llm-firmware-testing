// Traffic light with pedestrian request button (interrupt), walk beeps and an optional auto timer.
// Source: https://github.com/digitalkarl/Arduino-Traffic-Light-Project (Traffic_Light.ino), CC0-1.0.
// Refactored for testability (see module.yaml notes).
#pragma once
#include <Arduino.h>

// An auto timer option is enabled if pin10 is HIGH but if grounded then no auto timer
// Pedestrian Request Button is connected to Pin 2 via an interupt
// A Pedestrian acknowledgement LED is connected to Pin 12
// Speaker is connected to Pin 9

#define TrafficRed 3
#define TrafficAmber 4
#define TrafficGreen 5
#define pedestrianRed 7
#define pedestrianGreen 8
#define speakerPin 9
#define buttonPin 2 // pushbutton doesn't need hardware debouncing
#define pedestrianAcknowledgeLED_Red 12
#define pedestrianAcknowledgeLED_Green 11
#define autoSwitchPin 10

extern volatile int pinState;   // set HIGH by the button interrupt, reset after the pedestrian phase
extern int counter;             // walk-time loop counter (0..100)
extern long lastTime;           // time of the last pedestrian phase (auto timer)
extern long autoTimerDelay;     // 60000 ms
extern int duration, onTime, offTime;  // beep: duration cycles of onTime/offTime microseconds

void traffic_set_delay_scale(uint8_t percent);  // the 1 s and 100 ms delays of a cycle: 100 = original (default), 0 = none
void traffic_init();             // setup(): pins 3-12 outputs, button interrupt (pin 2, RISING), auto timer start
void traffic_loop();             // one loop() pass: auto timer (if pin 10 HIGH), pedestrian phase if requested, green
void traffic_loop_with(bool auto_switch_high, unsigned long now_ms);  // the same pass with the switch state and time given
void pressDetector();            // the interrupt routine (button press)
void trafficStop();              // red for traffic, green for pedestrians
void trafficAmber();
void trafficGreen();
