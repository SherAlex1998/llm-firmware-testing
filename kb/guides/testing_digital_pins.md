# Testing digital pins on the Arduino Uno

## Reading back an output pin
`digitalRead(pin)` reads the port input register (PINx), which reflects the actual level of the pin. For a pin
configured as `OUTPUT` it therefore returns the level the sketch last wrote with `digitalWrite()`, so a test can
check an output with `digitalRead()` right after calling the function under test.

On pins with a PWM timer (3, 5, 6, 9, 10, 11) both `digitalRead()` and `digitalWrite()` first switch the PWM
off (Arduino AVR core, `wiring_digital.c`). Reading a pin that is currently producing PWM stops the PWM.

## Inputs, buttons and floating pins
A pin configured as `INPUT` with nothing connected is floating: `digitalRead()` may return `HIGH` or `LOW`, and
the value can change at random. A test must not expect a particular value from an input it does not drive.

With `INPUT_PULLUP` the internal pull-up resistor holds the pin `HIGH`; a button wired to GND reads `LOW` when
pressed. With `INPUT` and an external pull-down resistor, a button wired to 5V reads `HIGH` when pressed.

To test logic that depends on an input (a button, a sensor), call the function that takes the input value as a
parameter, for example `update(bool pressed)`, with both values, instead of trying to change the physical pin.

## Pin numbers on the Uno
- Digital pins 0 to 13. The analog inputs A0 to A5 are also digital pins 14 to 19.
- Pins 0 and 1 are the Serial RX/TX lines; the test protocol uses Serial, so tests must not drive them.
- PWM pins: 3, 5, 6, 9, 10, 11. `LED_BUILTIN` is pin 13.
- Check outputs against the pin numbers in the module's specification (its pin table), not against constants
  taken from the code under test: a wrong pin number in the code is only detected if the test uses the pin
  number the hardware is actually wired to.

## Covering a function
- Call the module's init function at the start of every test so that tests do not depend on each other.
- For every output a function drives, assert each pin it should set and each pin it should leave unchanged.
- For functions with an index or a range argument (LED number, state number), test the first value, the last
  value and one value outside the range.
