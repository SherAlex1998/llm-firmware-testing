#include "logic.h"
#include <LiquidCrystal.h>
#include <EEPROM.h>
byte bat_full[8] = {
	B01110,
	B11111,
	B11111,
	B11111,
	B11111,
	B11111,
	B11111,
	B11111,
};

byte bat_high[8] = {
	B01110,
	B10001,
	B10001,
	B11111,
	B11111,
	B11111,
	B11111,
	B11111,
};

byte bat_low[8] = {
	B01110,
	B10001,
	B10001,
	B10001,
	B10001,
	B11111,
	B11111,
	B11111,
};

byte bat_empty[8] = {
	B01110,
	B10001,
	B10001,
	B10001,
	B10001,
	B10001,
	B10001,
	B11111,
};

State state;

static LiquidCrystal lcd(LCD_RS, LCD_E, LCD_DB4, LCD_DB5, LCD_DB6, LCD_DB7);
int alarm_minutes, alarm_seconds;
int minutes, seconds;
boolean first_press;
char time_string[6];

// Testability: scale of the blocking delays and a shadow of the 8x2 LCD (it cannot be read back)
static uint8_t delay_scale = 100;
static char shadow[2][9];
static uint8_t cur_col = 0, cur_row = 0;

static void paced_delay(unsigned long ms) {
	delay(ms * delay_scale / 100);
}

void ktimer_set_delay_scale(uint8_t percent) {
	delay_scale = percent;
}

static void lcd_clear() {
	lcd.clear();
	for (uint8_t r = 0; r < 2; r++) {
		memset(shadow[r], ' ', 8);
		shadow[r][8] = 0;
	}
	cur_col = cur_row = 0;
}

static void lcd_set_cursor(uint8_t col, uint8_t row) {
	lcd.setCursor(col, row);
	cur_col = col;
	cur_row = row;
}

static void lcd_print(const char *text) {
	lcd.print(text);
	for (uint8_t i = 0; text[i]; i++, cur_col++)
		if (cur_col < 8 && cur_row < 2) shadow[cur_row][cur_col] = text[i];
}

static void lcd_icon(uint8_t icon) {
	lcd.write(byte(icon));
	if (cur_col < 8 && cur_row < 3) shadow[cur_row][cur_col] = '0' + icon;
	cur_col++;
}

void ktimer_init() {
	pinMode(BUTTON1, INPUT);
	pinMode(BUTTON2, INPUT);

	lcd.begin(8, 2);
	lcd.createChar(BAT_FULL_ICON, bat_full);
	lcd.createChar(BAT_HIGH_ICON, bat_high);
	lcd.createChar(BAT_LOW_ICON, bat_low);
	lcd.createChar(BAT_EMPTY_ICON, bat_empty);
	lcd_clear();

	EEPROM.get(MINUTES_ADDRESS, alarm_minutes);
	EEPROM.get(SECONDS_ADDRESS, alarm_seconds);

	display_alarm_time();
	display_battery();

	first_press = true;

	state = IDLE;
	minutes = seconds = 0;
}

void ktimer_loop() {
	boolean b1 = state == IDLE && button_press(BUTTON1);
	boolean b2 = state == IDLE && !b1 && button_press(BUTTON2);
	ktimer_loop_with(b1, b2);
}

void ktimer_loop_with(boolean button1, boolean button2) {
	switch (state) {
	case IDLE:
		if (button1) {
			EEPROM.put(MINUTES_ADDRESS, alarm_minutes);
			EEPROM.put(SECONDS_ADDRESS, alarm_seconds);

			minutes = alarm_minutes;
			seconds = alarm_seconds;
			
			state = RUNNING;
		} else if (button2) {
			if (first_press) {
				alarm_minutes = 0;
				alarm_seconds = 0;
				first_press = false;
			}
			increase_alarm_time();
			lcd_clear();
			display_alarm_time();
			display_battery();
			paced_delay(100);
		}
		break;
	case RUNNING:
		lcd_clear();
		display_alarm_time();
		display_battery();
		lcd_set_cursor(0, 1);
		sprintf(time_string, "%02d:%02d", minutes, seconds);
		lcd_print(time_string);
		
		paced_delay(1000);
		count_down();
		
		if (minutes <= 0 && seconds <= 0) {
			lcd_clear();
			display_alarm_time();
			display_battery();
			lcd_set_cursor(0, 1);
			lcd_print("!!!!");
			alarm();
			
			state = IDLE;
		}
		break;
	default:
		lcd_clear();
		lcd_print("ERROR");
		break;
	}
}

void count_down(void) {
	seconds--;
	if (seconds < 0) {
		minutes--;
		seconds = 59;
	}
}

void display_alarm_time(void) {
	lcd_set_cursor(0, 0);
	sprintf(time_string, "%02d:%02d", alarm_minutes, alarm_seconds);
	lcd_print(time_string);
}

void alarm(void) {
	for (uint8_t j = 0; j < BEEP_GROUPS; j++) {
		for (uint8_t i = 0; i < BEEPS; i++) {
			tone(BUZZER, TONE_FREQ);
			paced_delay(BEEP_DELAY);
			noTone(BUZZER);
			paced_delay(BEEP_DELAY);
		}
		paced_delay(BEEP_GROUP_DELAY);
	}
}

void increase_alarm_time(void) {
	alarm_minutes++;
}

boolean button_press(uint8_t pin) {
	if (digitalRead(pin) == true) {
		delay(DEBOUNCE_MS);
		if (digitalRead(pin) == true) {
			return true;
		}
	}
	return false;
}

/* Return battery voltage * 100 (to deal with int instead of float) */
int battery_voltage(void) {
	float mult = (5.0 / 1023.0) * 100.0;
	float volt = (float) analogRead(BATTERY) * mult;
	return (int) volt;
}

void display_battery(void) {
	float voltage = battery_voltage();

	lcd_set_cursor(BAT_ICON_COL, BAT_ICON_ROW);
	
	if (voltage >= BAT_FULL_VOLT) {
		lcd_icon(BAT_FULL_ICON);
	} else if (voltage >= BAT_HIGH_VOLT) {
		lcd_icon(BAT_HIGH_ICON);
	} else if (voltage >= BAT_LOW_VOLT) {
		lcd_icon(BAT_LOW_ICON);
	} else {
		lcd_icon(BAT_EMPTY_ICON);
	}
}

const char* ktimer_lcd_line(uint8_t row) {
	return shadow[row % 2];
}
