// Hand-written infrastructure check (not shown to the LLM): init on avr8js, buttons released, merge logic.
#include "logic.h"
#include "test_runner.h"

TEST(init_places_two_tiles) {
  initGame();
  uint8_t tiles = 0;
  for (uint8_t x = 0; x < 4; x++) for (uint8_t y = 0; y < 4; y++) if (board[x][y]) tiles++;
  ASSERT_EQ(tiles, 2);
  ASSERT_FALSE(isPressed(buttonUp));
}

TEST(left_merges_equal_tiles) {
  initGame();
  initBoard();
  score = 0;
  board[1][0] = 2;
  board[3][0] = 2;
  ASSERT_TRUE(moveLeft());
  ASSERT_EQ(board[0][0], 4);
  ASSERT_EQ(score, 4);
}

TEST(step_with_direction_adds_a_tile) {
  initGame();
  initBoard();
  board[3][3] = 2;
  ASSERT_TRUE(gameStepWith(buttonUp));
  uint8_t tiles = 0;
  for (uint8_t x = 0; x < 4; x++) for (uint8_t y = 0; y < 4; y++) if (board[x][y]) tiles++;
  ASSERT_EQ(tiles, 2);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
