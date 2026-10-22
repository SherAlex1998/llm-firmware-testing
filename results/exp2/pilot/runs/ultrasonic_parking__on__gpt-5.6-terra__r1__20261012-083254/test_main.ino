#include "logic.h"
#include "test_runner.h"

TEST(park_init_resets_state_and_sets_purple) {
  park_init();
  sonar.applyReading(42);
  sonar.setTargetDistance(55);

  park_init();

  ASSERT_EQ(sonar.getTargetDistance(), 30);
  ASSERT_EQ(sonar.getDistance(), 0);
  ASSERT_EQ(sonar.getLastDistance(), 0);
  ASSERT_EQ(led.color(), 0x00FF00FF);
}

TEST(led_controller_sets_expected_colours) {
  park_init();

  led.rgb(12, 34, 56);
  ASSERT_EQ(led.color(), 0x000C2238);

  led.red();
  ASSERT_EQ(led.color(), 0x00FF0000);

  led.green();
  ASSERT_EQ(led.color(), 0x0000FF00);

  led.yellow();
  ASSERT_EQ(led.color(), 0x00C83200);

  led.purple();
  ASSERT_EQ(led.color(), 0x00FF00FF);

  led.off();
  ASSERT_EQ(led.color(), 0);
}

TEST(led_controller_flash_methods_finish_on_colour) {
  park_init();

  led.flashRed(20);
  ASSERT_EQ(led.color(), 0x00FF0000);

  led.flashGreen(20);
  ASSERT_EQ(led.color(), 0x0000FF00);
}

TEST(sonar_read_distance_reads_fixed_sensor) {
  park_init();

  int distance = sonar.readDistance();

  ASSERT_NEAR(distance, 40, 1);
  ASSERT_EQ(sonar.getDistance(), distance);
  ASSERT_EQ(sonar.getLastDistance(), 0);
  ASSERT_PIN(4, LOW);
}

TEST(sonar_apply_reading_tracks_previous_reading) {
  park_init();

  sonar.applyReading(40);
  ASSERT_EQ(sonar.getDistance(), 40);
  ASSERT_EQ(sonar.getLastDistance(), 0);

  sonar.applyReading(35);
  ASSERT_EQ(sonar.getDistance(), 35);
  ASSERT_EQ(sonar.getLastDistance(), 40);

  sonar.setTargetDistance(48);
  ASSERT_EQ(sonar.getTargetDistance(), 48);

  sonar.reset();
  ASSERT_EQ(sonar.getTargetDistance(), 30);
  ASSERT_EQ(sonar.getDistance(), 0);
  ASSERT_EQ(sonar.getLastDistance(), 0);
}

TEST(sonar_zone_boundaries_are_classified) {
  park_init();

  sonar.applyReading(46);
  ASSERT_TRUE(sonar.objectAwayFromTargetZone());
  ASSERT_FALSE(sonar.objectInTargetZone());

  sonar.applyReading(45);
  ASSERT_FALSE(sonar.objectAwayFromTargetZone());
  ASSERT_TRUE(sonar.objectInTargetZone());

  sonar.applyReading(25);
  ASSERT_TRUE(sonar.objectInTargetZone());
  ASSERT_TRUE(sonar.objectPastTargetZone());

  sonar.applyReading(24);
  ASSERT_FALSE(sonar.objectInTargetZone());
  ASSERT_TRUE(sonar.objectPastTargetZone());

  sonar.applyReading(14);
  ASSERT_TRUE(sonar.objectTooCloseToSensor());

  sonar.applyReading(500);
  ASSERT_TRUE(sonar.objectOutOfRange());
}

TEST(sonar_idle_timeout_detects_stillness_and_movement) {
  park_init();

  sonar.applyReading(40);
  ASSERT_FALSE(sonar.idleTimeoutReached(100));

  sonar.applyReading(40);
  ASSERT_TRUE(sonar.idleTimeoutReached(30101));

  sonar.applyReading(100);
  ASSERT_FALSE(sonar.idleTimeoutReached(40000));
}

TEST(set_led_color_matches_distance_zones) {
  park_init();

  sonar.applyReading(80);
  setLedColor();
  ASSERT_EQ(led.color(), 0x00FF00FF);

  sonar.applyReading(40);
  setLedColor();
  ASSERT_EQ(led.color(), 0x0000FF00);

  sonar.applyReading(20);
  setLedColor();
  ASSERT_EQ(led.color(), 0x00C83200);

  sonar.applyReading(10);
  setLedColor();
  ASSERT_EQ(led.color(), 0x00FF0000);
}

TEST(park_loop_with_updates_led_for_reading) {
  park_init();

  park_loop_with(40, false, 100);

  ASSERT_EQ(sonar.getDistance(), 40);
  ASSERT_EQ(sonar.getLastDistance(), 0);
  ASSERT_EQ(led.color(), 0x0000FF00);
}

TEST(park_loop_with_button_event_stores_target) {
  park_init();

  park_loop_with(40, true, 100);

  ASSERT_EQ(sonar.getTargetDistance(), 40);
  ASSERT_EQ(led.color(), 0x0000FF00);
}

TEST(park_loop_uses_sensor_reading_and_sets_green) {
  park_init();

  park_loop();

  ASSERT_NEAR(sonar.getDistance(), 40, 1);
  ASSERT_EQ(led.color(), 0x0000FF00);
}

TEST(button_is_not_pressed_while_released) {
  park_init();

  ASSERT_FALSE(setupButton.wasPressed());
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
