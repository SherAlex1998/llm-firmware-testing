// Hand-written infrastructure check (not shown to the LLM): HC-SR04 at 40 cm, button released, zone logic.
#include "logic.h"
#include "test_runner.h"

TEST(sensor_reads_fixed_distance) {
  park_init();
  ASSERT_NEAR(sonar.readDistance(), 40, 2);
  ASSERT_FALSE(setupButton.wasPressed());
}

TEST(in_target_zone_is_green) {
  park_init();
  park_loop_with(32, false, 1000);
  ASSERT_TRUE(sonar.objectInTargetZone());
  ASSERT_EQ(led.color(), 0x00FF00UL);
}

TEST(button_stores_target) {
  park_init();
  park_loop_with(80, true, 1000);
  ASSERT_EQ(sonar.getTargetDistance(), 80);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
