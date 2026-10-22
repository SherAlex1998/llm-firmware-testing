#include "logic.h"
#include "test_runner.h"

TEST(init_state) {
  gate_init();
  ASSERT_PIN(4, HIGH);   // red LED on
  ASSERT_PIN(2, LOW);    // green LED off
  ASSERT_FALSE(gate_is_open());
  ASSERT_FALSE(gate_object_between());
  ASSERT_EQ(gate_servo1_angle(), 180);
  ASSERT_EQ(gate_servo2_angle(), 90);
}

TEST(open_moves_servos_and_leds) {
  gate_init();
  gate_open();
  ASSERT_TRUE(gate_is_open());
  ASSERT_PIN(2, HIGH);   // green on
  ASSERT_PIN(4, LOW);    // red off
  ASSERT_EQ(gate_servo1_angle(), 90);
  ASSERT_EQ(gate_servo2_angle(), 180);
}

TEST(close_after_open_restores_state) {
  gate_init();
  gate_open();
  gate_close();
  ASSERT_FALSE(gate_is_open());
  ASSERT_PIN(4, HIGH);
  ASSERT_PIN(2, LOW);
  ASSERT_EQ(gate_servo1_angle(), 180);
  ASSERT_EQ(gate_servo2_angle(), 90);
}

TEST(update_object_enters_opens_gate) {
  gate_init();
  gate_update(true, false);
  ASSERT_TRUE(gate_object_between());
  ASSERT_TRUE(gate_is_open());
}

TEST(update_object_leaves_closes_gate) {
  gate_init();
  gate_update(true, false);   // object enters, gate opens
  gate_update(false, true);   // object leaves, gate closes
  ASSERT_FALSE(gate_object_between());
  ASSERT_FALSE(gate_is_open());
}

TEST(update_sensor2_only_keeps_closed) {
  gate_init();
  gate_update(false, true);   // no object between, sensor2 alone triggers close (already closed)
  ASSERT_FALSE(gate_is_open());
  ASSERT_FALSE(gate_object_between());
  ASSERT_PIN(4, HIGH);
  ASSERT_PIN(2, LOW);
}

TEST(sensor_detected_reads_pin_level) {
  gate_init();
  pinMode(A0, OUTPUT);
  digitalWrite(A0, LOW);
  ASSERT_TRUE(gate_sensor1_detected());
  digitalWrite(A0, HIGH);
  ASSERT_FALSE(gate_sensor1_detected());
}

TEST(poll_reads_sensors_and_opens_gate) {
  gate_init();
  pinMode(A0, OUTPUT);
  pinMode(A1, OUTPUT);
  digitalWrite(A0, LOW);   // object present at sensor1
  digitalWrite(A1, HIGH);  // sensor2 clear
  gate_poll();
  ASSERT_TRUE(gate_is_open());
  ASSERT_TRUE(gate_object_between());
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
