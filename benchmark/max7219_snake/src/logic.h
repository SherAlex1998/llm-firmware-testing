// Snake on four chained MAX7219 8x8 LED matrices (32x8 field) with an analog joystick.
// Source: https://github.com/nazarii-piontko/arduino-snake, MIT License, Copyright (c) 2020 Nazarii Piontko.
// Refactored for testability (see module.yaml notes). Game rules: game.h / game.cpp.
#pragma once
#include <Arduino.h>
#include "game.h"

#define LED_PIN_DATA 12
#define LED_PIN_CLOCK 10
#define LED_PIN_CS 11

#define LED_OP_DECODEMODE 9
#define LED_OP_INTENSITY 10
#define LED_OP_SCANLIMIT 11
#define LED_OP_SHUTDOWN 12
#define LED_OP_DISPLAYTEST 15

#define BTN_PIN_DY A0
#define BTN_PIN_DX A1
#define BTN_SIGNAL_MID_LEVEL 518
#define BTN_SIGNAL_THRESHOLD (BTN_SIGNAL_MID_LEVEL / 2)

#define LOOP_DELAY 25          // ms per loop pass (the delay itself is in the sketch)
#define GAME_LOOP_DELAY 250    // ms per game step = 10 loop passes
#define RENDER_PANELS 4

void snake_init();                         // setup(): random seed from A0, new game and level, display init, first frame
void snake_init_seeded(unsigned long seed);  // the same with an explicit random seed != 0 (repeatable blocks and food)
void snake_input_read();                   // reads the joystick (A0, A1) into the next move
void snake_input_apply(int raw_dy, int raw_dx);  // the same for given ADC readings (518 = centre)
void snake_loop_pass();                    // one loop() pass: input, every 10th pass a game step
STEP_RESULT snake_step();                  // one game step; on failure a new level starts; the field is redrawn
void snake_render_init();                  // MAX7219 setup commands to all panels
void snake_render_field();                 // sends the whole field to the matrices

DIRECTION snake_next_move();
Game* snake_game();                        // the game state used by the sketch
int snake_loop_filter();                   // loop passes since the last game step
uint8_t snake_led_row(uint8_t panel, uint8_t row);  // last pattern sent for field row 0..7 of a panel: panel 3 shows x 0..7, panel 0 shows x 24..31; bit 7 = leftmost x
