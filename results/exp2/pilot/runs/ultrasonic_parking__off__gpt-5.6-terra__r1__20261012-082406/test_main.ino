#include "logic.h"
#include "test_runner.h"

TEST(init_resets_state_and_sets_purple) {
  park_init();

  ASSERT_EQ(sonar.getTargetDistance(), DEFAULT_TARGET_DISTANCE_CM);
  ASSERT_EQ(sonar.getDistance(), 0);
  ASSERT_EQ(sonar.getLastDistance(), 0);
  ASSERT_EQ(led.color(), 0x00FF00FF);
}

TEST(reading_history_target_and_reset) {
  park_init();

  sonar.applyReading(35);
  ASSERT_EQ(sonar.getDistance(), 35);
  ASSERT_EQ(sonar.getLastDistance(), 0);

  sonar.applyReading(45);
  ASSERT_EQ(sonar.getDistance(), 45);
  ASSERT_EQ(sonar.getLastDistance(), 35);

  sonar.setTargetDistance(55);
  ASSERT_EQ(sonar.getTargetDistance(), 55);

  sonar.reset();
  ASSERT_EQ(sonar.getTargetDistance(), DEFAULT_TARGET_DISTANCE_CM);
  ASSERT_EQ(sonar.getDistance(), 0);
  ASSERT_EQ(sonar.getLastDistance(), 0);
}

TEST(distance_zone_predicates_cover_boundaries) {
  park_init();
  sonar.setTargetDistance(30);

  sonar.applyReading(46);
  ASSERT_TRUE(sonar.objectAwayFromTargetZone());
  ASSERT_FALSE(sonar.objectInTargetZone());

  sonar.applyReading(45);
  ASSERT_FALSE(sonar.objectAwayFromTargetZone());
  ASSERT_TRUE(sonar.objectInTargetZone());

  sonar.applyReading(24);
  ASSERT_TRUE(sonar.objectPastTargetZone());
  ASSERT_FALSE(sonar.objectTooCloseToSensor());

  sonar.applyReading(14);
  ASSERT_TRUE(sonar.objectTooCloseToSensor());
  ASSERT_FALSE(sonar.objectPastTargetZone());

  sonar.applyReading(MAX_DISTANCE_CM);
  ASSERT_TRUE(sonar.objectOutOfRange());
  ASSERT_FALSE(sonar.objectAwayFromTargetZone());
}

TEST(idle_timeout_turns_on_after_stationary_period) {
  park_init();

  sonar.applyReading(40);
  ASSERT_FALSE(sonar.idleTimeoutReached(0));

  sonar.applyReading(40);
  ASSERT_FALSE(sonar.idleTimeoutReached(30000));
  ASSERT_TRUE(sonar.idleTimeoutReached(30001));
}

TEST(movement_resets_idle_timeout) {
  park_init();

  sonar.applyReading(40);
  ASSERT_FALSE(sonar.idleTimeoutReached(0));

  sonar.applyReading(40);
  ASSERT_TRUE(sonar.idleTimeoutReached(30001));

  sonar.applyReading(100);
  ASSERT_FALSE(sonar.idleTimeoutReached(30002));
  ASSERT_FALSE(sonar.idleTimeoutReached(30003));
}

TEST(led_controller_colours_and_flash_results) {
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

  led.flashRed(0);
  ASSERT_EQ(led.color(), 0x00FF0000);

  led.flashGreen(0);
  ASSERT_EQ(led.color(), 0x0000FF00);

  led.off();
  ASSERT_EQ(led.color(), 0);
}

TEST(set_led_color_selects_each_distance_zone) {
  park_init();

  sonar.applyReading(46);
  setLedColor();
  ASSERT_EQ(led.color(), 0x00FF00FF);

  sonar.applyReading(40);
  setLedColor();
  ASSERT_EQ(led.color(), 0x0000FF00);

  sonar.applyReading(24);
  setLedColor();
  ASSERT_EQ(led.color(), 0x00C83200);

  sonar.applyReading(14);
  setLedColor();
  ASSERT_EQ(led.color(), 0x00FF0000);
}

TEST(loop_with_idles_and_wakes_on_movement) {
  park_init();

  park_loop_with(40, false, 0);
  ASSERT_EQ(led.color(), 0x0000FF00);

  park_loop_with(40, false, 30001);
  ASSERT_EQ(led.color(), 0);

  park_loop_with(100, false, 30002);
  ASSERT_EQ(led.color(), 0x00FF00FF);
}

TEST(loop_with_button_sets_target_and_flashes_green) {
  park_init();

  park_loop_with(40, true, 0);

  ASSERT_EQ(sonar.getTargetDistance(), 40);
  ASSERT_EQ(led.color(), 0x0000FF00);
}

TEST(hardware_loop_reads_fixed_sonar_and_sets_green) {
  park_init();

  park_loop();

  ASSERT_NEAR(sonar.getDistance(), 40, 3);
  ASSERT_EQ(led.color(), 0x0000FF00);
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
