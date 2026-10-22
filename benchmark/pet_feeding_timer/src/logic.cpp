//Include and initialize the LCD screen
//lcd(rs, enable, d4, d5, d6, d7)
#include "logic.h"
#include <LiquidCrystal.h>
static LiquidCrystal lcd(4, 5, 6, 7, 8, 9);

//Pin layout
//Note: Buttons need to be on interrupt enabled pins (2&3 on UNO and Nano)

//Feeding Time intervals
const unsigned long hour = 3600000;   // 3600000 milliseconds in an hour
const unsigned long minute = 60000;   // 60000 milliseconds in a minute
const unsigned long interval1 = 6*hour;   // maybe a treat after 6 hours (yellow LED)
const unsigned long interval2 = 9*hour;   // should feed again after 9 hours (red LED)
const unsigned long screenDelay = 7000;   // Turn screen Off after 7s delay
const unsigned long redrawInterval = 800;   //Redraw screen every 0.8 seconds
volatile unsigned long lastFeedingTime = 0;

//Screen On intervals
volatile unsigned long screenOnTime = 0;
int screenState = 0;   // 1 = backlight on

//Button Timings (for debouncing)
volatile unsigned long previousButtonTime;
volatile unsigned long buttonTime;
volatile unsigned long previousScreenOnButtonTime;
volatile unsigned long screenOnButtonTime;

//Screen Timing
volatile unsigned long previousRedrawTime = 0;

// Testability: shadow of the 16x2 LCD (it cannot be read back)
static char shadow[2][17];
static uint8_t cur_col = 0, cur_row = 0;

static void lcd_set_cursor(uint8_t col, uint8_t row) {
  lcd.setCursor(col, row);
  cur_col = col;
  cur_row = row;
}

static void lcd_print(const String &text) {
  lcd.print(text);
  for (unsigned int i = 0; i < text.length(); i++, cur_col++)
    if (cur_col < 16 && cur_row < 2) shadow[cur_row][cur_col] = text[i];
}

void pet_reset_state() {
  lastFeedingTime = 0;
  screenOnTime = 0;
  screenState = 0;
  previousButtonTime = buttonTime = previousScreenOnButtonTime = screenOnButtonTime = 0;
  previousRedrawTime = 0;
  for (uint8_t r = 0; r < 2; r++) {
    memset(shadow[r], ' ', 16);
    shadow[r][16] = 0;
  }
}

const char *pet_lcd_line(uint8_t row) {
  return shadow[row % 2];
}

void pet_init() {
  pet_reset_state();
  // Set up pin modes, LED -> OUT, buttons -> IN + set pull-up resistors + interrupts
  pinMode(redLEDPin,OUTPUT);
  pinMode(yellowLEDPin,OUTPUT);
  pinMode(greenLEDPin,OUTPUT);
  pinMode(screenOnPin,OUTPUT);
  pinMode(buttonPin, INPUT);
  pinMode(screenOnButtonPin, INPUT);
  digitalWrite(buttonPin, HIGH);
  digitalWrite(screenOnButtonPin, HIGH);
  attachInterrupt(digitalPinToInterrupt(buttonPin), resetTime, FALLING);
  attachInterrupt(digitalPinToInterrupt(screenOnButtonPin), screenON, FALLING);

  // Columns and rows of LCD screen
  screenState = 1;
  lcd.begin(16, 2);
  lcd_set_cursor(1,0);
  lcd_print("Eraser was fed");

   // Debug and communication
   //Serial.begin(9600);
}

void pet_loop() {
  pet_loop_at(millis());
}

void pet_loop_at(unsigned long currentTime) {
  unsigned long deltaTime = currentTime - lastFeedingTime;

  // Check Screen state and light up backlight accordingly
  if (screenState == 1 && currentTime - screenOnTime < screenDelay) {
    digitalWrite(screenOnPin, HIGH);
    //Serial.println("Screen ON");
  }
  else {
    digitalWrite(screenOnPin, LOW);
    //Serial.println("Screen OFF");
    screenState = 0;
  }

  ledFeedingStatus(deltaTime);

  displayFeedingInfo(deltaTime,currentTime);

}

void resetTime() {
  resetTimeAt(millis());
}

void resetTimeAt(unsigned long now) {
  buttonTime = now;

  if (buttonTime - previousButtonTime < 250) {
    //Debouncing
  }
  else if (buttonTime - previousButtonTime < 1000) {
    //Serial.println("-------Feed Button Press---------");
    //Successive clicks of the feeding button will set the time backward an hour
    lastFeedingTime = lastFeedingTime - 1*hour;
    previousButtonTime = buttonTime;
    screenState = 1;
    screenOnTime = buttonTime;
  }
  else {
    //Serial.println("-------Feed Button Press---------");
    //First click of the feeding button resets time
    lastFeedingTime = buttonTime;
    previousButtonTime = buttonTime;
    screenState = 1;
    screenOnTime = buttonTime;
  }
}

void screenON() {
  screenONAt(millis());
}

void screenONAt(unsigned long now) {
  screenOnButtonTime = now;

  if (screenOnButtonTime - previousScreenOnButtonTime > 250) {
    //Serial.println("-------Screen Button Press---------");
    if (screenState == 1) {
      screenState = 0;
    }
    else if (screenState == 0) {
      screenState = 1;
      screenOnTime = screenOnButtonTime;
    }
    previousScreenOnButtonTime = screenOnButtonTime;
  }
}

void displayFeedingInfo(unsigned long deltaTime, unsigned long currentTime) {
  //Print the last feeding time on the LCD
  // Redraw only every redrawInterval to avoid flickering

  if (currentTime - previousRedrawTime > redrawInterval) {
  
    int hours = deltaTime / hour;
    int minutes = (deltaTime % hour) / minute;

    if (deltaTime / minute < 2) {
      lcd_set_cursor(0,1);
      lcd_print("                ");
      lcd_set_cursor(4,1);
      lcd_print("Just now");
    }
    else if (deltaTime / hour == 0) {
      lcd_set_cursor(0,1);
      lcd_print("                ");
      lcd_set_cursor(1, 1);
      lcd_print(String(minutes)+" minutes ago");
    }
    else if (deltaTime / hour == 1) {
      lcd_set_cursor(0,1);
      lcd_print("                ");
      lcd_set_cursor(3,1);
      lcd_print(String(hours)+" hour ago");
    }
    else {
      lcd_set_cursor(0,1);
      lcd_print("                ");
      lcd_set_cursor(3,1);
      lcd_print(String(hours)+" hours ago");
    }

  previousRedrawTime = currentTime;
  }
}

void ledFeedingStatus(unsigned long deltaTime) {
  // Check Time and light up LEDs accordingly
  if (deltaTime > interval2) {
    digitalWrite(greenLEDPin, LOW);
    digitalWrite(yellowLEDPin, LOW);
    digitalWrite(redLEDPin, HIGH);
  }
  else if (deltaTime > interval1) {
    digitalWrite(greenLEDPin, LOW);
    digitalWrite(yellowLEDPin, HIGH);
    digitalWrite(redLEDPin, LOW);
  }
  else {
    digitalWrite(greenLEDPin, HIGH);
    digitalWrite(yellowLEDPin, LOW);
    digitalWrite(redLEDPin, LOW);
  }
}

