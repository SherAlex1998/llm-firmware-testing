// Hand-written infrastructure check (not shown to the LLM): joystick at rest, game logic and render shadow reachable.
#include "logic.h"
#include "test_runner.h"

TEST(joystick_at_rest_keeps_direction) {
  snake_init_seeded(7);
  snake_input_read();
  ASSERT_EQ(snake_next_move(), DIRECTION_RIGHT);
}

TEST(snake_starts_in_centre_and_moves_right) {
  snake_init_seeded(7);
  ASSERT_EQ(snake_game()->snake.first_point.x, 16);
  ASSERT_EQ(snake_game()->snake.first_point.y, 4);
  ASSERT_EQ(snake_step(), STEP_RESULT_SUCCESS);
  ASSERT_EQ(snake_game()->snake.first_point.x, 17);
  ASSERT_TRUE((snake_led_row(1, 4) & 0x40) != 0);   // x = 17 is panel 1, bit 6
}

TEST(joystick_up_changes_move) {
  snake_init_seeded(7);
  snake_input_apply(0, 518);
  ASSERT_EQ(snake_next_move(), DIRECTION_UP);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
