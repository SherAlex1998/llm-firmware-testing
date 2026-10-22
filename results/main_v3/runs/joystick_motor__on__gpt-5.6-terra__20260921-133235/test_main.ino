#include "logic.h"
#include "test_runner.h"

TEST(init_sets_clockwise_and_stops) {
  jm_init();
  ASSERT_TRUE(jm_direction());
  ASSERT_EQ(jm_speed(), 0);
  ASSERT_PIN(3, HIGH);
  ASSERT_PIN(4, LOW);
  ASSERT_PIN(5, LOW);
}

TEST(direction_controls_both_direction_pins) {
  jm_init();

  jm_set_direction(false);
  ASSERT_FALSE(jm_direction());
  ASSERT_PIN(3, LOW);
  ASSERT_PIN(4, HIGH);

  jm_set_direction(true);
  ASSERT_TRUE(jm_direction());
  ASSERT_PIN(3, HIGH);
  ASSERT_PIN(4, LOW);
}

TEST(speed_clamps_and_reports_written_value) {
  jm_init();

  jm_set_speed(-20);
  ASSERT_EQ(jm_speed(), 0);
  ASSERT_PIN(5, LOW);
  ASSERT_PIN(3, HIGH);
  ASSERT_PIN(4, LOW);

  jm_set_speed(128);
  ASSERT_EQ(jm_speed(), 128);
  ASSERT_TRUE(jm_direction());

  jm_set_speed(300);
  ASSERT_EQ(jm_speed(), 255);
  ASSERT_PIN(5, HIGH);
  ASSERT_PIN(3, HIGH);
  ASSERT_PIN(4, LOW);
}

TEST(stop_sets_speed_to_zero) {
  jm_init();
  jm_set_speed(255);
  jm_stop();

  ASSERT_EQ(jm_speed(), 0);
  ASSERT_PIN(5, LOW);
  ASSERT_PIN(3, HIGH);
  ASSERT_PIN(4, LOW);
}

TEST(axis_conversion_functions_handle_center_and_ends) {
  jm_init();

  ASSERT_EQ(jm_speed_from_axis(512), 0);
  ASSERT_EQ(jm_speed_from_axis(513), 0);
  ASSERT_EQ(jm_speed_from_axis(532), 10);
  ASSERT_EQ(jm_speed_from_axis(0), 256);
  ASSERT_EQ(jm_speed_from_axis(1023), 255);

  ASSERT_FALSE(jm_clockwise_from_axis(512));
  ASSERT_FALSE(jm_clockwise_from_axis(0));
  ASSERT_TRUE(jm_clockwise_from_axis(513));
  ASSERT_TRUE(jm_clockwise_from_axis(1023));
}

TEST(apply_axis_stops_in_dead_zone_and_drives_outside) {
  jm_init();

  jm_set_direction(false);
  jm_set_speed(255);
  jm_apply_axis(520);
  ASSERT_EQ(jm_speed(), 0);
  ASSERT_FALSE(jm_direction());
  ASSERT_PIN(5, LOW);
  ASSERT_PIN(3, LOW);
  ASSERT_PIN(4, HIGH);

  jm_apply_axis(0);
  ASSERT_EQ(jm_speed(), 255);
  ASSERT_FALSE(jm_direction());
  ASSERT_PIN(5, HIGH);
  ASSERT_PIN(3, LOW);
  ASSERT_PIN(4, HIGH);

  jm_apply_axis(1023);
  ASSERT_EQ(jm_speed(), 255);
  ASSERT_TRUE(jm_direction());
  ASSERT_PIN(5, HIGH);
  ASSERT_PIN(3, HIGH);
  ASSERT_PIN(4, LOW);
}

TEST(update_keeps_state_and_direction_outputs_consistent) {
  jm_init();
  jm_update();

  ASSERT_TRUE(jm_speed() >= 0 && jm_speed() <= 255);

  if (jm_direction()) {
    ASSERT_PIN(3, HIGH);
    ASSERT_PIN(4, LOW);
  } else {
    ASSERT_PIN(3, LOW);
    ASSERT_PIN(4, HIGH);
  }

  if (jm_speed() == 0) {
    ASSERT_PIN(5, LOW);
  } else if (jm_speed() == 255) {
    ASSERT_PIN(5, HIGH);
  }
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
