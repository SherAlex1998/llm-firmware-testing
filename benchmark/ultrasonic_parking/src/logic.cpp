#include "logic.h"

SonarController sonar = SonarController(TRIGGER_PIN, ECHO_PIN);
LedController led = LedController(LED_PIN);
ButtonController setupButton = ButtonController(SETUP_BUTTON_PIN);

void park_init()
{
  sonar.reset();
  led.purple();
}

static void process(int distance, bool pressed, unsigned long now)
{
  if (sonar.idleTimeoutReached(now)) {
    led.off();
  } else {
    setLedColor();
  }

  if (pressed) {
    sonar.setTargetDistance(distance);
    for (int i = 0; i < 3; i++) {
      led.flashGreen(50);
    }
  }
}

void park_loop() {
  int distance = sonar.readDistance();
  process(distance, setupButton.wasPressed(), millis());
}

void park_loop_with(int distance, bool button_released, unsigned long now_ms) {
  sonar.applyReading(distance);
  process(distance, button_released, now_ms);
}

void setLedColor() {
  if (sonar.objectAwayFromTargetZone()) {
    led.purple();
  } else if (sonar.objectInTargetZone()) {
    led.green();
  } else if (sonar.objectPastTargetZone()) {
    led.yellow();
  } else if (sonar.objectTooCloseToSensor()) {
    led.flashRed(100);
  }
}
