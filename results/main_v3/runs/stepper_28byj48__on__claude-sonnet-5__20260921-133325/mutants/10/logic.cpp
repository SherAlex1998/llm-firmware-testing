#include "logic.h"

// Coil energising order for one electrical cycle (bit0 = IN1 ... bit3 = IN4).
static const uint8_t k_steps[STEPPER_SEQUENCE_LENGTH] = {
  B1000, B1100, B0100, B0110, B0010, B0011, B0001, B1001
};

static int8_t g_phase = 0;
static long g_position = 0;
static unsigned int g_delay_ms = STEPPER_DEFAULT_DELAY_MS;

void stepper_init() {
  pinMode(STEPPER_PIN_IN1, OUTPUT);
  pinMode(STEPPER_PIN_IN2, OUTPUT);
  pinMode(STEPPER_PIN_IN3, OUTPUT);
  pinMode(STEPPER_PIN_IN4, OUTPUT);
  g_phase = 0;
  g_position = 0;
  g_delay_ms = STEPPER_DEFAULT_DELAY_MS;
  stepper_release();
}

void stepper_set_delay(unsigned int ms) {
  g_delay_ms = ms;
}

unsigned int stepper_delay() {
  return g_delay_ms;
}

uint8_t stepper_coil_pattern(uint8_t phase) {
  if (phase >= STEPPER_SEQUENCE_LENGTH) {
    return 0;
  }
  return k_steps[phase];
}

bool stepper_apply_phase(uint8_t phase) {
  if (phase >= STEPPER_SEQUENCE_LENGTH) {
    return false;
  }
  uint8_t pattern = k_steps[phase];
  digitalWrite(STEPPER_PIN_IN1, bitRead(pattern, 0));
  digitalWrite(STEPPER_PIN_IN2, bitRead(pattern, 1));
  digitalWrite(STEPPER_PIN_IN3, bitRead(pattern, 2));
  digitalWrite(STEPPER_PIN_IN4, bitRead(pattern, 6));
  g_phase = (int8_t)phase;
  return true;
}

int8_t stepper_phase() {
  return g_phase;
}

long stepper_position() {
  return g_position;
}

void stepper_step_full(int steps) {
  while (steps > 0) {
    if (g_phase % 2 != 1) {
      g_phase++;          // move onto an odd (two-coil) phase
    } else {
      g_phase += 2;       // advance two positions per full step
    }
    if (g_phase > 7) {
      g_phase = 1;
    }
    stepper_apply_phase(g_phase);
    delay(g_delay_ms);
    g_position += 2;
    steps--;
  }
  while (steps < 0) {
    if (g_phase % 2 != 1) {
      g_phase--;
    } else {
      g_phase -= 2;
    }
    if (g_phase < 1) {
      g_phase = 7;
    }
    stepper_apply_phase(g_phase);
    delay(g_delay_ms);
    g_position -= 2;
    steps++;
  }
}

void stepper_step_half(int steps) {
  while (steps > 0) {
    g_phase++;
    if (g_phase > 7) {
      g_phase = 0;
    }
    stepper_apply_phase(g_phase);
    delay(g_delay_ms);
    g_position++;
    steps--;
  }
  while (steps < 0) {
    g_phase--;
    if (g_phase < 0) {
      g_phase = 7;
    }
    stepper_apply_phase(g_phase);
    delay(g_delay_ms);
    g_position--;
    steps++;
  }
}

void stepper_release() {
  digitalWrite(STEPPER_PIN_IN1, LOW);
  digitalWrite(STEPPER_PIN_IN2, LOW);
  digitalWrite(STEPPER_PIN_IN3, LOW);
  digitalWrite(STEPPER_PIN_IN4, LOW);
}
