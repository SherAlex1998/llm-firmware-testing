#include "logic.h"

static int g_led_state = HIGH;
static int g_button_state = LOW;
static int g_last_reading = LOW;
static unsigned long g_last_debounce_ms = 0;
static unsigned long g_debounce_delay_ms = DB_DEFAULT_DEBOUNCE_MS;

void db_init() {
  pinMode(DB_PIN_BUTTON, INPUT);
  pinMode(DB_PIN_LED, OUTPUT);
  g_led_state = HIGH;
  g_button_state = LOW;
  g_last_reading = LOW;
  g_last_debounce_ms = 0;
  g_debounce_delay_ms = DB_DEFAULT_DEBOUNCE_MS;
  ;
}

void db_update(int reading, unsigned long now_ms) {
  if (reading != g_last_reading) {
    g_last_debounce_ms = now_ms;     // reading changed: restart the debounce timer
  }
  if ((now_ms - g_last_debounce_ms) > g_debounce_delay_ms) {
    if (reading != g_button_state) {
      g_button_state = reading;
      if (g_button_state == HIGH) {
        g_led_state = !g_led_state;  // toggle only on press
      }
    }
  }
  digitalWrite(DB_PIN_LED, g_led_state);
  g_last_reading = reading;
}

void db_poll() {
  db_update(digitalRead(DB_PIN_BUTTON), millis());
}

int db_led_state() {
  return g_led_state;
}

int db_button_state() {
  return g_button_state;
}

unsigned long db_debounce_delay() {
  return g_debounce_delay_ms;
}

void db_set_debounce_delay(unsigned long ms) {
  g_debounce_delay_ms = ms;
}

unsigned long db_last_change_ms() {
  return g_last_debounce_ms;
}
