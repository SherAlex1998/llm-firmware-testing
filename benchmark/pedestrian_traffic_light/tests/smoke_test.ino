// Hand-written infrastructure check (not shown to the LLM): no request by default, ISR twin, pedestrian phase.
#include "logic.h"
#include "test_runner.h"

TEST(idle_pass_shows_green) {
  traffic_set_delay_scale(0);
  traffic_init();
  traffic_loop_with(false, 1000);
  ASSERT_PIN(TrafficGreen, HIGH);
  ASSERT_PIN(pedestrianRed, HIGH);
  ASSERT_EQ(pinState, LOW);
}

TEST(request_runs_pedestrian_phase) {
  traffic_set_delay_scale(0);
  traffic_init();
  pressDetector();
  ASSERT_PIN(pedestrianAcknowledgeLED_Red, HIGH);
  traffic_loop_with(false, 2000);
  ASSERT_EQ(pinState, LOW);
  ASSERT_EQ(counter, 0);
  ASSERT_PIN(pedestrianAcknowledgeLED_Green, HIGH);
  ASSERT_PIN(TrafficGreen, HIGH);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
