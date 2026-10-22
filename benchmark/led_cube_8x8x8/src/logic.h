// 8x8x8 LED cube driven over SPI through a chain of 74HC595 shift registers: eight animations, a button to
// switch, automatic rotation every 5 s. Source: https://github.com/itsharryle/LED_CUBE (CUBE/CUBE.ino),
// MIT License, Copyright (c) 2017 VectStudio. Refactored for testability (see module.yaml notes).
#pragma once
#include <Arduino.h>

#define XAXIS 0
#define YAXIS 1
#define ZAXIS 2
#define POS_X 0
#define NEG_X 1
#define POS_Z 2
#define NEG_Z 3
#define POS_Y 4
#define NEG_Y 5
#define BUTTON_PIN 8
#define RED_LED 5
#define GREEN_LED 7
#define TOTAL_EFFECTS 8
#define RAIN 0
#define PLANE_BOING 1
#define SEND_VOXELS 2
#define WOOP_WOOP 3
#define CUBE_JUMP 4
#define GLOW 5
#define TEXT 6
#define LIT 7
#define RAIN_TIME 260
#define PLANE_BOING_TIME 220
#define SEND_VOXELS_TIME 140
#define WOOP_WOOP_TIME 350
#define CUBE_JUMP_TIME 200
#define GLOW_TIME 8
#define TEXT_TIME 300
#define CLOCK_TIME 500

// cube[layer][row] holds 8 voxels as bits (see setVoxel/getVoxel); one layer per SPI frame in renderCube()
extern uint8_t characters[10][8];
extern uint8_t cube[8][8];
extern uint8_t currentEffect;
extern uint16_t timer;
extern uint64_t randomTimer;
extern bool autoRotate;
extern uint64_t lastEffectChange;
extern uint32_t effectDuration;
extern bool loading;
extern uint8_t planePosition;
extern uint8_t planeDirection;
extern bool looped;
extern uint8_t selX;
extern uint8_t selY;
extern uint8_t selZ;
extern uint8_t sendDirection;
extern bool sending;
extern uint8_t cubeSize;
extern bool cubeExpanding;
extern uint8_t xPos;
extern uint8_t yPos;
extern uint8_t zPos;
extern bool glowing;
extern uint16_t glowCount;
extern uint8_t charCounter;
extern uint8_t charPosition;
extern uint8_t delay_scale;

void cube_set_delay_scale(uint8_t percent);   // 0.5 s pause on effect change: 100 = original (default), 0 = none
void cube_reset_state();                      // cube, effect and timers back to power-up values (cube_init calls it)
void cube_init();                             // setup(): SPI, button (pin 8, INPUT_PULLUP, LOW = pressed), status LEDs
void cube_loop();                             // one loop() pass reading millis() and the button
void cube_loop_with(unsigned long now, bool buttonPressed);
void renderCube();
void rain();
void planeBoing();
void sendVoxels();
void woopWoop();
void cubeJump();
void glow();
void text(char string[], uint8_t len);
void lit();
void setVoxel(uint8_t x, uint8_t y, uint8_t z);
void clearVoxel(uint8_t x, uint8_t y, uint8_t z);
bool getVoxel(uint8_t x, uint8_t y, uint8_t z);
void setPlane(uint8_t axis, uint8_t i);
void shift(uint8_t dir);
void drawCube(uint8_t x, uint8_t y, uint8_t z, uint8_t s);
void lightCube();
void clearCube();
