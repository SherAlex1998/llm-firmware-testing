#include "logic.h"
#include "test_runner.h"

TEST(init_sets_forward_direction_and_zero_power) {
  motor_init();
  ASSERT_TRUE(motor_direction());
  ASSERT_EQ(motor_power(), 0);
  ASSERT_PIN(12, LOW);
  ASSERT_PIN(13, HIGH);
  ASSERT_PIN(11, LOW);
}

TEST(direction_controls_direction_pins) {
  motor_init();

  motor_set_direction(false);
  ASSERT_FALSE(motor_direction());
  ASSERT_PIN(12, HIGH);
  ASSERT_PIN(13, LOW);

  motor_set_direction(true);
  ASSERT_TRUE(motor_direction());
  ASSERT_PIN(12, LOW);
  ASSERT_PIN(13, HIGH);
}

TEST(power_is_clamped_and_extremes_drive_enable) {
  motor_init();

  motor_set_power(-10);
  ASSERT_EQ(motor_power(), 0);
  ASSERT_PIN(11, LOW);

  motor_set_power(128);
  ASSERT_EQ(motor_power(), 128);

  motor_set_power(300);
  ASSERT_EQ(motor_power(), 255);
  ASSERT_PIN(11, HIGH);
}

TEST(reading_to_power_maps_and_clamps) {
  motor_init();
  ASSERT_EQ(motor_power_from_reading(-1), 0);
  ASSERT_EQ(motor_power_from_reading(0), 0);
  ASSERT_EQ(motor_power_from_reading(512), 127);
  ASSERT_EQ(motor_power_from_reading(1023), 255);
  ASSERT_EQ(motor_power_from_reading(2000), 255);
}

TEST(stop_disables_motor_without_changing_direction_state) {
  motor_init();
  motor_set_direction(false);
  motor_set_power(255);

  motor_stop();

  ASSERT_FALSE(motor_direction());
  ASSERT_EQ(motor_power(), 0);
  ASSERT_PIN(12, LOW);
  ASSERT_PIN(13, LOW);
  ASSERT_PIN(11, LOW);
}

TEST(update_applies_potentiometer_reading) {
  motor_init();
  motor_set_power(255);

  motor_update();

  int expected = motor_power_from_reading(analogRead(A0));
  ASSERT_EQ(motor_power(), expected);
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
