# Testing code that talks to I2C, SPI and 1-Wire devices

## Is the device there?
On I2C (`Wire`, pins A4 = SDA, A5 = SCL on the Uno) a device answers its address:
`Wire.beginTransmission(addr); byte err = Wire.endTransmission();` returns 0 when the device acknowledged,
2 for a NACK on the address. Character LCD backpacks usually sit at 0x27 or 0x3F, DS1307/DS3231 clocks at 0x68.
On 1-Wire (`OneWire`) `ds.search(addr)` returns false when no device is found and the first byte of the
address is the family code (0x28 for DS18B20, 0x10 for DS18S20); `OneWire::crc8(addr, 7)` must equal `addr[7]`.

## Write-only displays
Character LCDs, 7-segment drivers (TM1637, MAX7219) and LED matrices are written to but usually not read back
by the sketch, so a test cannot ask the display what it shows. Check what the code wrote instead: buffers or
strings the code keeps, state variables that select the screen, or values a library keeps in RAM
(for example `Adafruit_NeoPixel::getPixelColor(i)` or `MD_MAX72XX::getColumn(c)`).

## Sensor readings
A sensor reading depends on the environment, so assert ranges rather than exact values unless the value is
known (`ASSERT_NEAR(reading, expected, tolerance)`). The first conversion of some sensors returns a power-on
value (DS18B20: 85.0 degC) before a real conversion has completed. Ultrasonic distance from an echo pulse
is `duration_us / 58` cm; `pulseIn()` returns 0 when no echo arrives within the timeout.

## Clocks
An RTC keeps counting while the test runs: read the time once, compare hours/minutes with tolerance, or write a
known time first (`setDateTime`/raw register writes) and read it back. DS1307 and DS3231 store time in BCD in
registers 0x00-0x06; DS1307 also has 56 bytes of battery-backed RAM at 0x08-0x3F.

## SPI
`SPI.transfer(b)` sends a byte and returns the byte clocked in at the same time; with nothing driving MISO the
returned value is meaningless. Chip select (SS/CS) must be LOW during a transfer; drivers that cascade several
chips (74HC595, MAX7219) shift all bytes, then pulse the latch/CS once.
