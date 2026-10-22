# Testing PWM, analog inputs and servos

## analogWrite and what a test can observe
`analogWrite(pin, value)` sets the pin to `OUTPUT`. `analogWrite(pin, 0)` is implemented as
`digitalWrite(pin, LOW)` and `analogWrite(pin, 255)` as `digitalWrite(pin, HIGH)`; any other value starts the
PWM timer (Arduino AVR core, `wiring_analog.c`). On the Uno the PWM frequency is 490 Hz, and 980 Hz on pins 5
and 6.

Only the two extreme values can be checked with `digitalRead()`. For values 1 to 254 the duty cycle cannot be
read back, and `digitalRead()` would switch the PWM off. Check intermediate levels through the module's own
state: a getter that returns the level it wrote, or the value returned by the function that computes the level.

## Functions that convert readings
`analogRead()` returns 0 to 1023. An unconnected analog input is floating, so tests should not depend on what
`analogRead()` returns. Test the conversion functions (raw reading to speed, angle, direction) directly with
explicit values: 0, 1023, the midpoint 512, and the values right below and above every threshold in the code.

`map(value, fromLow, fromHigh, toLow, toHigh)` uses integer arithmetic and truncates; `constrain()` clamps. When a
function combines them, test the ends of the input range and one value in the middle.

## Servo library
- On the Uno the Servo library uses Timer1 and disables `analogWrite()` on pins 9 and 10, whether or not a servo
  is attached to them.
- `Servo::write(angle)` clamps angles below 0 to 0 and above 180 to 180; values of 544 and more are treated as
  a pulse width in microseconds (Servo 1.3.0, `Servo.cpp`).
- `Servo::read()` converts the last pulse width back to degrees, so it returns the last angle written; it does
  not measure the position of the shaft. After `attach()` and before any `write()` the pulse is 1500 us.
- A test for a servo module can call the function that moves the servo and check `read()` or the module's angle
  getter; for a sweep, check the first, the last and one intermediate angle, and that the angle never leaves
  the allowed range.

## Stepper motors
A stepper driven through four pins (for example a 28BYJ-48 via ULN2003) is controlled by writing a coil
pattern to the four pins for every step. Test the pattern table and the step function: after each step exactly
the expected coils are `HIGH`, the step counter changes by one in the right direction, and the sequence wraps
around after the last phase. Releasing the motor should set all four pins `LOW`.
