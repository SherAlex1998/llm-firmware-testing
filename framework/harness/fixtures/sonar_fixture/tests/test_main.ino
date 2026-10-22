// Passes only when an HC-SR04 at 100 cm is simulated (Wokwi with the module's diagram.json).
#include "logic.h"
#include "test_runner.h"

TEST(reads_fixed_distance) {
  sonar_init();
  ASSERT_NEAR(sonar_read_cm(), 100, 5);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
