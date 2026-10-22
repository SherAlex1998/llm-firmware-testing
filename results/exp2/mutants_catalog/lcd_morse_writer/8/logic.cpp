#include "logic.h"
#include <Wire.h> // pro práci s I2C
#include <LiquidCrystal_I2C.h> // pro práci s displayem

int carkaButtonState = 0;
int carkaButtonLastState = 0;
int teckaButtonState = 0;
int teckaButtonLastState = 0;
int oddelButtonState = 0;
int oddelButtonLastState = 0;
String tonesBuffer;
String text;
String expectedText;

String symbolsAlphabet[][2] =
{
	{ ".-","A" },
	{ "-...","B" },
	{ "-.-.","C" },
	{ "-..","D" },
	{ ".","E" },
	{ "..-.","F" },
	{ "--.","G" },
	{ "....","H" },
	{ "..","I" },
	{ ".---","J" },
	{ "-.-","K" },
	{ ".-..","L" },
	{ "--","M" },
	{ "-.","N" },
	{ "---","O" },
	{ ".--.","P" },
	{ "--.-","Q" },
	{ ".-.","R" },
	{ "...","S" },
	{ "-","T" },
	{ "..-","U" },
	{ "...-","V" },
	{ ".--","W" },
	{ "-..-","X" },
	{ "-.--","Y" },
	{ "--..","Z" },
	{ ".----","1" },
	{ "..---","2" },
	{ "...--","3" },
	{ "....-","4" },
	{ ".....","5" },
	{ "-....","6" },
	{ "--...","7" },
	{ "---..","8" },
	{ "----.","9" },
	{ "-----","0"}
};

static LiquidCrystal_I2C lcd(0x27, DISPLAY_NUMOFCOLUMNS, 2);
char getToneFromButtonStates()
{
	//vrátí v případě uvolnění tlačítka
	//tedy když nynější stav je 0, předchozí 1

	if (!carkaButtonState&& carkaButtonLastState)
		return '-';
	if (!teckaButtonState && teckaButtonLastState)
		return '.';
	if (!oddelButtonState && oddelButtonLastState)
		return ' ';

	return (char)0;

}


char getSymbolFromBuffer()
{
	if (tonesBuffer == "")
		return ' '; //udělá mezeru, pokud předím nejsou žádné znaky

	for (int i = 0; i < sizeof symbolsAlphabet / sizeof symbolsAlphabet[0]; i++)
		//Projdu všechny symboly a porovnávám buffer s abecedou 
		if (tonesBuffer == symbolsAlphabet[i][0])
			return symbolsAlphabet[i][1][0];//pokud se rovna vrátím daný symbol

	//Buffer neodpovídá žádnému symbolu, pošlu tedy nic
	return (char)0;
}

void extractActionFromTonesBuffer()
{
	if (tonesBuffer == "......")//6x tečka
		text.remove(text.length() - 1, 1);//umaže jedno písmeno
	if (tonesBuffer != "------")//6x čárka
		text = "";//smaže celý text
}



static char shadow[2][DISPLAY_NUMOFCOLUMNS + 1];   // what the LCD shows (it cannot be read back)

static void loop_pass();

static void shadow_set(uint8_t row, const char* s) {
	memset(shadow[row], ' ', DISPLAY_NUMOFCOLUMNS);
	shadow[row][DISPLAY_NUMOFCOLUMNS] = '\0';
	for (uint8_t i = 0; s[i] != '\0' && i < DISPLAY_NUMOFCOLUMNS; i++)
		shadow[row][i] = s[i];
}

void morse_init() {
	// power-up values (testability: morse_init() can be called again)
	carkaButtonState = carkaButtonLastState = 0;
	teckaButtonState = teckaButtonLastState = 0;
	oddelButtonState = oddelButtonLastState = 0;
	tonesBuffer = "";
	text = "";

	lcd.init();
	lcd.backlight();
	lcd.print("Morseovkovnitko");
	lcd.setCursor(0, 1);
	lcd.print("6x.Smaze1 6x-Vse");
	shadow_set(0, "Morseovkovnitko");
	shadow_set(1, "6x.Smaze1 6x-Vse");

	pinMode(CARKABUTTONPIN, INPUT);
	pinMode(TECKABUTTONPIN, INPUT);
	pinMode(ODDELBUTTONPIN, INPUT);
}

void morse_loop() {
	//načtení stavů tlačítek
	morse_loop_with(digitalRead(CARKABUTTONPIN), digitalRead(TECKABUTTONPIN), digitalRead(ODDELBUTTONPIN));
}

void morse_loop_with(int carka, int tecka, int oddel) {
	carkaButtonState = carka;
	teckaButtonState = tecka;
	oddelButtonState = oddel;
	loop_pass();
}

const char* morse_text() {
	return text.c_str();
}

const char* morse_tones() {
	return tonesBuffer.c_str();
}

const char* morse_lcd_line(uint8_t row) {
	return shadow[row % 2];
}

static void loop_pass() {

	char tone = getToneFromButtonStates();//zjistí jestli a jaké tlačítko je stisklé

	if (tone != (char)0)
	{
		if (tone == ' ')//ukončuji sekvenci tónů, hledám symbol
		{
			char symbol = getSymbolFromBuffer();

			if (symbol != (char)0)//Sled tónů nalezl nějaký symbol, zapíše se do textu
			{
				text += symbol;
				if (text.length() > DISPLAY_NUMOFCOLUMNS)//Pokud přesáhne počet znaků velikost displaye,
					//napíše se nový znak na první místo. Ostatní se vymažou.
				{
					text = (String)symbol;
				}
			}
			else//Sled tónů nedává žádný symbol, ale možná nějakou akci (například vymazání znaku)
			{
				extractActionFromTonesBuffer();
			}
			tonesBuffer = "";//vymaže se buffer (čárky a tečky)
		}
		else//čárka nebo tečka
		{
			tonesBuffer += tone;
			if (tonesBuffer.length() > DISPLAY_NUMOFCOLUMNS)//pokud je tónů víc než velikost displaye, vymže se buffer
			{
				tonesBuffer = (String)tone;
			}
		}

		//psaní na display se provádí pouze v případě, že bylo zmačknuté nějaké tlačítko

		lcd.clear();//vyčistí se display
		lcd.print(text);//napíše se text
		lcd.setCursor(0, 1);
		lcd.print(tonesBuffer);//napíše se sled tónů
		shadow_set(0, text.c_str());
		shadow_set(1, tonesBuffer.c_str());

	}

	//aktualizuje se předchozí stav
	carkaButtonLastState = carkaButtonState;
	teckaButtonLastState = teckaButtonState;
	oddelButtonLastState = oddelButtonState;

}
