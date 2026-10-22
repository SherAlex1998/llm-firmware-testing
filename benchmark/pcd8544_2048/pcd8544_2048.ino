// Arduino library
#include <Arduino.h>

// Configuations and core logics are in src/ (logic.cpp)
#include "src/logic.h"

// == Main routines ==
// Get the show on!
void setup() {
	initGame();
}

// one pass of the game loop (the original called loopGame(), which loops forever, from setup())
void loop() {
	gameStep();
}
