// Hand-written infrastructure check (not shown to the LLM): LCD on I2C, button released, ISR flag starts the game.
#include "logic.h"
#include "test_runner.h"
#include <Wire.h>

TEST(lcd_answers_and_start_screen) {
  runner_set_delay_scale(0);
  runner_init();
  Wire.beginTransmission(0x27);
  ASSERT_EQ(Wire.endTransmission(), 0);
  runner_loop();
  ASSERT_FALSE(playing);
  ASSERT_FALSE(buttonPushed);
}

TEST(button_starts_and_game_advances) {
  runner_set_delay_scale(0);
  runner_init();
  buttonPush();
  runner_loop();
  ASSERT_TRUE(playing);
  runner_loop();
  ASSERT_EQ(distance, 1);
  ASSERT_EQ(CARPos, CAR_POSITION_RUN_LOWER_2);
}

TEST(collision_with_block_ends_game) {
  runner_set_delay_scale(0);
  runner_init();
  initializeGraphics();
  terrainLower[CAR_HORIZONTAL_POSITION] = SPRITE_TERRAIN_SOLID;
  ASSERT_TRUE(drawCAR(CAR_POSITION_RUN_LOWER_1, terrainUpper, terrainLower, 0));
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
