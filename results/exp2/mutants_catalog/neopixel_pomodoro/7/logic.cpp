#include "logic.h"
#include <Adafruit_NeoPixel.h>

static Adafruit_NeoPixel ring = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

State currentState;
int pomodoro;
unsigned long starttime;
unsigned long duration;
static bool buttonState = HIGH;

// Testability: time of the current loop pass (the original called millis() in place) and a scale for the
// blocking animations.
static unsigned long g_now = 0;
static uint8_t delay_scale = 100;

static void paced_delay(unsigned long ms)
{
  delay(ms * delay_scale / 100);
}

static void blink(int ledcount);
static void initLeds();

void neo_set_delay_scale(uint8_t percent)
{
  delay_scale = percent;
}

void neo_init()
{
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  ring.begin();
  ring.show();
  buttonState = HIGH;
  starttime = 0;
  duration = 0;
  g_now = millis();

  turnOff();
}

void neo_loop()
{
  ButtonClick button = getButtonClick();
  neo_loop_at(millis(), button);
}

void neo_loop_at(unsigned long now, ButtonClick button)
{
  g_now = now;

  // Time can be extended by pressing the button. 1 short click => +1 minute.
  if (currentState != Off && button == ShortClick)
    starttime = min(starttime + 60000, now);

  // Long clicks can be used to turn the device on and off
  else if (button == LongClick)
  {
    if (currentState == Off)
      startPomodoro();
    else
      turnOff();
  }

  // If time runs out, go to next phase.
  else
  {
    unsigned long elapsedtime = now - starttime;
    if (elapsedtime > duration)
    {
      if (currentState == Pomodoro)
        startPause();
      else if (currentState == ShortPause || currentState == LongPause)
        startPomodoro();
    }
  }

  // Update ring
  updateDisplay();
}

void turnOff()
{
  pomodoro = 0;
  currentState = Off;
}

void startPomodoro()
{
  pomodoro = (pomodoro % 4) + 1;
  blink((PIXEL_COUNT * pomodoro) / 4);
  starttime = g_now;
  duration = POMODORO_TIME * 60000;
  currentState = Pomodoro;
  initLeds();
}

void startPause()
{
  starttime = g_now;
  if (pomodoro < 4)
  {
    duration = SHORT_PAUSE * 60000;
    currentState = ShortPause;
  }
  else
  {
    duration = LONG_PAUSE * 60000;
    currentState = LongPause;
  }
  initLeds();
}

ButtonClick getButtonClick()
{
  ButtonClick result = None;
  bool newState = digitalRead(BUTTON_PIN);
  if (newState == LOW && buttonState != HIGH)
  {
    delay(20);
    buttonState = digitalRead(BUTTON_PIN);
    if (buttonState == LOW)
    {
      for(int i = 0; i < 25; i++)
      {
        delay(20);
        buttonState = digitalRead(BUTTON_PIN);
        if (buttonState == HIGH)
          return ShortClick;
      }
      return LongClick;
    }
    else
      return None;
  }
  else
  {
    buttonState = newState;
    return None;
  }
}

void updateDisplay()
{
  float progress = ((float)(g_now - starttime) / duration) * PIXEL_COUNT;
  int done = floor(progress);
  float fraction = progress - done;
  for(int i = 0; i < PIXEL_COUNT; i++)
  {
    uint8_t intensity = 0;
    if (i == done)
      intensity = LUMINANCE * (1 - fraction);
    else if (i > done)
      intensity = LUMINANCE;

    if (currentState == Pomodoro)
      ring.setPixelColor(i, intensity, 0, 0);
    else if (currentState == ShortPause)
      ring.setPixelColor(i, intensity, intensity, 0);
    else if (currentState == LongPause)
      ring.setPixelColor(i, 0, intensity, 0);
    else
      ring.setPixelColor(i, 0, 0, 0);
  }
  ring.show();
}

static void blink(int ledcount)
{
  for(int i = 0; i < 3; i++)
  {
    for(int j = 0; j < PIXEL_COUNT; j++)
      ring.setPixelColor(j, 0, 0, j < ledcount ? LUMINANCE : 0);
    ring.show();
    paced_delay(200);
    for(int j = 0; j < PIXEL_COUNT; j++)
      ring.setPixelColor(j, 0, 0, 0);
    ring.show();
    paced_delay(200);
  }
}

static void initLeds()
{
  uint32_t color = 0;
  if (currentState == Pomodoro)
    color = ring.Color(LUMINANCE, 0, 0);
  else if (currentState == ShortPause)
    color = ring.Color(LUMINANCE, LUMINANCE, 0);
  else if (currentState == LongPause)
    color = ring.Color(0, LUMINANCE, 0);

  for(int i = PIXEL_COUNT; i > 0; i--)
  {
    ring.setPixelColor(i - 1, color);
    ring.show();
    paced_delay(20);
  }
}

uint32_t neo_pixel(uint8_t i)
{
  return ring.getPixelColor(i % PIXEL_COUNT);
}
