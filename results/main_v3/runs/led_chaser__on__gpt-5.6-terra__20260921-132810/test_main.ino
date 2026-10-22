#include "logic.h"
#include "test_runner.h"

TEST(init_sets_all_leds_off) {
  chaser_init();

  ASSERT_EQ(chaser_position(), -1);
  ASSERT_EQ(chaser_direction(), 1);
  ASSERT_EQ(chaser_count_on(), 0);
  ASSERT_PIN(2, LOW);
  ASSERT_PIN(3, LOW);
  ASSERT_PIN(4, LOW);
  ASSERT_PIN(5, LOW);
  ASSERT_PIN(6, LOW);
  ASSERT_PIN(7, LOW);
  ASSERT_PIN(8, LOW);
  ASSERT_PIN(9, LOW);
  ASSERT_PIN(10, LOW);
  ASSERT_PIN(11, LOW);
}

TEST(light_only_first_led) {
  chaser_init();

  ASSERT_TRUE(chaser_light_only(0));
  ASSERT_EQ(chaser_position(), 0);
  ASSERT_EQ(chaser_count_on(), 1);
  ASSERT_PIN(2, HIGH);
  ASSERT_PIN(3, LOW);
  ASSERT_PIN(4, LOW);
  ASSERT_PIN(5, LOW);
  ASSERT_PIN(6, LOW);
  ASSERT_PIN(7, LOW);
  ASSERT_PIN(8, LOW);
  ASSERT_PIN(9, LOW);
  ASSERT_PIN(10, LOW);
  ASSERT_PIN(11, LOW);
}

TEST(light_only_last_led) {
  chaser_init();

  ASSERT_TRUE(chaser_light_only(9));
  ASSERT_EQ(chaser_position(), 9);
  ASSERT_EQ(chaser_count_on(), 1);
  ASSERT_PIN(2, LOW);
  ASSERT_PIN(3, LOW);
  ASSERT_PIN(4, LOW);
  ASSERT_PIN(5, LOW);
  ASSERT_PIN(6, LOW);
  ASSERT_PIN(7, LOW);
  ASSERT_PIN(8, LOW);
  ASSERT_PIN(9, LOW);
  ASSERT_PIN(10, LOW);
  ASSERT_PIN(11, HIGH);
}

TEST(invalid_light_request_preserves_state) {
  chaser_init();
  ASSERT_TRUE(chaser_light_only(2));

  ASSERT_FALSE(chaser_light_only(10));
  ASSERT_EQ(chaser_position(), 2);
  ASSERT_EQ(chaser_count_on(), 1);
  ASSERT_PIN(2, LOW);
  ASSERT_PIN(3, LOW);
  ASSERT_PIN(4, HIGH);
  ASSERT_PIN(5, LOW);
  ASSERT_PIN(6, LOW);
  ASSERT_PIN(7, LOW);
  ASSERT_PIN(8, LOW);
  ASSERT_PIN(9, LOW);
  ASSERT_PIN(10, LOW);
  ASSERT_PIN(11, LOW);
}

TEST(step_starts_at_first_and_moves_forward) {
  chaser_init();

  chaser_step();
  ASSERT_EQ(chaser_position(), 0);
  ASSERT_EQ(chaser_direction(), 1);
  ASSERT_EQ(chaser_count_on(), 1);
  ASSERT_PIN(2, HIGH);
  ASSERT_PIN(3, LOW);
  ASSERT_PIN(4, LOW);
  ASSERT_PIN(5, LOW);
  ASSERT_PIN(6, LOW);
  ASSERT_PIN(7, LOW);
  ASSERT_PIN(8, LOW);
  ASSERT_PIN(9, LOW);
  ASSERT_PIN(10, LOW);
  ASSERT_PIN(11, LOW);

  chaser_step();
  ASSERT_EQ(chaser_position(), 1);
  ASSERT_EQ(chaser_direction(), 1);
  ASSERT_EQ(chaser_count_on(), 1);
  ASSERT_PIN(2, LOW);
  ASSERT_PIN(3, HIGH);
  ASSERT_PIN(4, LOW);
  ASSERT_PIN(5, LOW);
  ASSERT_PIN(6, LOW);
  ASSERT_PIN(7, LOW);
  ASSERT_PIN(8, LOW);
  ASSERT_PIN(9, LOW);
  ASSERT_PIN(10, LOW);
  ASSERT_PIN(11, LOW);
}

TEST(step_bounces_from_last_led) {
  chaser_init();
  ASSERT_TRUE(chaser_light_only(9));

  chaser_step();

  ASSERT_EQ(chaser_position(), 8);
  ASSERT_EQ(chaser_direction(), -1);
  ASSERT_EQ(chaser_count_on(), 1);
  ASSERT_PIN(2, LOW);
  ASSERT_PIN(3, LOW);
  ASSERT_PIN(4, LOW);
  ASSERT_PIN(5, LOW);
  ASSERT_PIN(6, LOW);
  ASSERT_PIN(7, LOW);
  ASSERT_PIN(8, LOW);
  ASSERT_PIN(9, LOW);
  ASSERT_PIN(10, HIGH);
  ASSERT_PIN(11, LOW);
}

TEST(step_bounces_from_first_led) {
  chaser_init();
  ASSERT_TRUE(chaser_light_only(9));

  for (uint8_t i = 0; i < 10; i++) {
    chaser_step();
  }

  ASSERT_EQ(chaser_position(), 1);
  ASSERT_EQ(chaser_direction(), 1);
  ASSERT_EQ(chaser_count_on(), 1);
  ASSERT_PIN(2, LOW);
  ASSERT_PIN(3, HIGH);
  ASSERT_PIN(4, LOW);
  ASSERT_PIN(5, LOW);
  ASSERT_PIN(6, LOW);
  ASSERT_PIN(7, LOW);
  ASSERT_PIN(8, LOW);
  ASSERT_PIN(9, LOW);
  ASSERT_PIN(10, LOW);
  ASSERT_PIN(11, LOW);
}

TEST(all_off_clears_active_led) {
  chaser_init();
  ASSERT_TRUE(chaser_light_only(5));

  chaser_all_off();

  ASSERT_EQ(chaser_position(), -1);
  ASSERT_EQ(chaser_count_on(), 0);
  ASSERT_PIN(2, LOW);
  ASSERT_PIN(3, LOW);
  ASSERT_PIN(4, LOW);
  ASSERT_PIN(5, LOW);
  ASSERT_PIN(6, LOW);
  ASSERT_PIN(7, LOW);
  ASSERT_PIN(8, LOW);
  ASSERT_PIN(9, LOW);
  ASSERT_PIN(10, LOW);
  ASSERT_PIN(11, LOW);
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
