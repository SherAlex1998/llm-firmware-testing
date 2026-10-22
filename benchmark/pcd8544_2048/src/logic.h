// 2048 tile game on a Nokia 5110 (PCD8544) display with four buttons; the game is saved to EEPROM.
// Source: https://github.com/zhaofengli/2048-arduino, BSD 2-Clause License, Copyright (c) 2014, Zhaofeng Li.
// Refactored for testability (see module.yaml notes). The game code lives in header files (base.h, board.h,
// display.h, button.h, random.h, format.h, gamesave.h, battery.h, serial.h, config.h) that define their functions,
// so only logic.cpp includes them; this header declares what tests use.
#pragma once
#include <Arduino.h>

#define buttonUp 0
#define buttonLeft 1
#define buttonDown 2
#define buttonRight 3
#define pinButtonUp    10   // INPUT_PULLUP, LOW = pressed
#define pinButtonLeft  8
#define pinButtonDown  7
#define pinButtonRight 9
#define saveThreshold 512

extern unsigned int board[4][4];   // board[x][y], 0 = empty, otherwise the tile value
extern unsigned long score;
extern unsigned int lastLargestTile;
extern bool backlightOn;

void initGame();                   // setup(): buttons, LCD, debouncer, board with two random tiles, first screen
void gameStep();                   // one pass of the game loop: a button press moves the tiles
bool gameStepWith(int direction);  // the same for a given direction; true if the board changed
void afterMove();                  // new tile, redraw, save to EEPROM when a new largest tile >= saveThreshold
void initBoard();                  // all tiles 0
void placeNewTile();               // a 2 (90 %) or 4 at a random free place
bool moveUp();                     // move/merge all tiles; true if anything moved (score grows by merged values)
bool moveLeft();
bool moveDown();
bool moveRight();
unsigned char moveOnce(unsigned char x, unsigned char y, bool h, bool v, bool hv, bool vv, bool noMerge);
unsigned char getStatus();         // 0 free tiles left, 1 full but a move is possible, 2 game over
unsigned int getLargestTile();
bool isCoordValid(unsigned char x, unsigned char y);
bool isPressed(unsigned char button);  // true once when a button is first pressed
bool buttonControl();
bool saveGame(unsigned int board[][4]);      // true if saved (EEPROM)
bool loadGame(unsigned int board[][4]);      // true if a save existed
char *getTileString(char *buffer, unsigned int value);  // tile text as shown on the display (format.h), buffer >= 4 chars
