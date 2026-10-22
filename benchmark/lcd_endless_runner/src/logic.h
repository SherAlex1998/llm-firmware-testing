// Endless runner game on a 16x2 I2C LCD: a car jumps over blocks with one button (interrupt), score = distance/8.
// Source: https://github.com/jaswch/Arduino_LCD_16x2_Endless_Runner_Game (game/game.ino), MIT License,
// (c) jaswch. Refactored for testability (see module.yaml notes).
#pragma once
#include <Arduino.h>

#define PIN_BUTTON 2
#define PIN_AUTOPLAY 3   // was pin 1 (Serial TX); output HIGH when the lane ahead is free
#define SPRITE_RUN1 1  //Car sprite
#define SPRITE_RUN2 2
#define SPRITE_JUMP 3
#define SPRITE_JUMP_UPPER '.'         
#define SPRITE_JUMP_LOWER 4
#define SPRITE_TERRAIN_EMPTY ' '      
#define SPRITE_TERRAIN_SOLID 5
#define SPRITE_TERRAIN_SOLID_RIGHT 6
#define SPRITE_TERRAIN_SOLID_LEFT 7
#define CAR_HORIZONTAL_POSITION 1    // Horizontal position of CAR on screen
#define TERRAIN_WIDTH 16
#define TERRAIN_EMPTY 0
#define TERRAIN_LOWER_BLOCK 1
#define TERRAIN_UPPER_BLOCK 2
#define CAR_POSITION_OFF 0          // CAR is invisible
#define CAR_POSITION_RUN_LOWER_1 1  // CAR is running on lower row (pose 1)
#define CAR_POSITION_RUN_LOWER_2 2  //                              (pose 2)
#define CAR_POSITION_JUMP_1 3       // Starting a jump
#define CAR_POSITION_JUMP_2 4       // Half-way up
#define CAR_POSITION_JUMP_3 5       // Jump is on upper row
#define CAR_POSITION_JUMP_4 6       // Jump is on upper row
#define CAR_POSITION_JUMP_5 7       // Jump is on upper row
#define CAR_POSITION_JUMP_6 8       // Jump is on upper row
#define CAR_POSITION_JUMP_7 9       // Half-way down
#define CAR_POSITION_JUMP_8 10      // About to land
#define CAR_POSITION_RUN_UPPER_1 11 // CAR is running on upper row (pose 1)
#define CAR_POSITION_RUN_UPPER_2 12 //                              (pose 2)

extern char terrainUpper[TERRAIN_WIDTH + 1];   // sprites of the upper row
extern char terrainLower[TERRAIN_WIDTH + 1];
extern volatile bool buttonPushed;            // set by the button interrupt
extern byte CARPos;                           // CAR_POSITION_*
extern byte newTerrainType, newTerrainDuration;
extern bool playing, blink;
extern unsigned int distance;                 // score shown = distance >> 3

void runner_set_delay_scale(uint8_t percent);  // 100 ms blink delay of the start screen: 100 = original (default), 0 = none
void runner_reset_state();                    // former loop() statics and flags back to power-up values
void runner_init();                           // setup(): button + interrupt 0 (pin 2, FALLING), autoplay pin, graphics, LCD
void runner_loop();                           // one loop() pass: start screen or one game step
void initializeGraphics();                    // custom characters, empty terrain
void advanceTerrain(char* terrain, byte newTerrain);  // slide one row left by half a character
bool drawCAR(byte position, char* terrainUpper, char* terrainLower, unsigned int score);  // draws, true on collision
void buttonPush();                            // the interrupt routine
const char *runner_lcd_line(uint8_t row);     // LCD row 0/1, 16 characters (custom sprites appear as codes 1..7)
