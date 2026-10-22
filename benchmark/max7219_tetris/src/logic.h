// Tetris on two MAX7219 8x8 LED matrices (8 x 16 field) with an analog joystick.
// Source: https://github.com/vahidzee/aurdoino-tetris (main/main.ino), MIT License, Copyright (c) 2019 Vahid Zehtab.
// Refactored for testability (see module.yaml notes).
#pragma once
#include <Arduino.h>

// Screen pins (moved off pins 0/1, which carry Serial)
#define SCREEN_DIN_PIN 12
#define SCREEN_CS_PIN 10
#define SCREEN_CLK_PIN 11

// Joystick pins
#define JYS_X_PIN 0       // A0
#define JYS_Y_PIN 1       // A1
#define JYS_BTN_PIN 3
#define JYS_VCC_PIN 5     // powers the joystick (OUTPUT HIGH)

#define DEVICE_WIDTH 8
#define DEVICE_HEIGHT 8
#define SCREEN_HEIGHT 16
#define SCREEN_WIDTH 8

typedef struct {
    byte x, y;
} Point;

typedef struct {
    Point points[4];
    short type;        // 0 I, 1 O, 2..4 T/L shapes, 5 S/Z shape
} Block;

// Game state (globals of the original sketch); y = 0 is the bottom row
extern bool bottom_chunk[SCREEN_WIDTH][SCREEN_HEIGHT];
extern Block live_block;
extern int loop_counter;

void tetris_set_delay_scale(uint8_t percent);  // 5 s load screen and 1 s game-over pause: 100 = original (default), 0 = none
void tetris_init();                     // setup(): joystick, display, load screen, new game
void tetris_loop_pass();                // one loop() pass: gravity every 3rd pass, joystick action, redraw, landing
void tetris_apply_joystick(int x, int y);  // action for given A0/A1 readings (centre about 512)

void init_led_controller();
void init_joy_stick();
void show_screen_for_map(bool pixels[SCREEN_WIDTH][SCREEN_HEIGHT]);
void show_game_map();
void show_load_screen();
bool live_block_reached_bottom_chunck();
void register_live_block_to_bottom_chunck();
void move_live_block_left();
void move_live_block_right();
void move_live_block_down();
void rotatelive_block();
void clear_bottom_chunck();
void start_new_game();
void generate_new_live_block();
void remove_full_bottom_rows();
void getAction();                       // reads the joystick and applies the action

bool tetris_shown(uint8_t x, uint8_t y);  // pixel last sent to the matrices (the display cannot be read back)
