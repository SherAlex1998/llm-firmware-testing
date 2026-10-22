// Hand-written infrastructure check (not shown to the LLM): joystick at rest, game logic and frame shadow reachable.
#include "logic.h"
#include "test_runner.h"

TEST(joystick_at_rest_does_nothing) {
  tetris_set_delay_scale(0);
  tetris_init();
  Block before = live_block;
  getAction();
  for (int i = 0; i < 4; i++) {
    ASSERT_EQ(live_block.points[i].x, before.points[i].x);
    ASSERT_EQ(live_block.points[i].y, before.points[i].y);
  }
}

TEST(full_bottom_row_is_removed) {
  tetris_set_delay_scale(0);
  tetris_init();
  for (int x = 0; x < SCREEN_WIDTH; x++) bottom_chunk[x][0] = true;
  bottom_chunk[2][1] = true;
  remove_full_bottom_rows();
  ASSERT_TRUE(bottom_chunk[2][0]);
  ASSERT_FALSE(bottom_chunk[0][0]);
}

TEST(frame_shows_live_block) {
  tetris_set_delay_scale(0);
  tetris_init();
  show_game_map();
  ASSERT_TRUE(tetris_shown(live_block.points[0].x, live_block.points[0].y));
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
