# Testing interrupt handlers and EEPROM-backed state

## Interrupt service routines
`attachInterrupt(digitalPinToInterrupt(pin), isr, mode)` works on pins 2 and 3 of the Uno (interrupts 0 and 1);
for other pins `digitalPinToInterrupt()` returns `NOT_AN_INTERRUPT`. The handler is a plain `void f()` function:
a test can call it directly to simulate the event, then check the flags or counters it changed. Variables shared
with an ISR are declared `volatile`; multi-byte ones should be read with interrupts disabled
(`noInterrupts(); copy = value; interrupts();`). `millis()` does not advance inside an ISR and `delay()` must not
be used there.

## Debouncing and edges
Code that reacts to a press often acts on the edge (previous level HIGH, current LOW) and ignores changes within
a debounce window measured with `millis()`. A test therefore has to provide two readings (before and after)
and, if the code compares times, times that are far enough apart.

## EEPROM
The ATmega328P has 1024 bytes of EEPROM (`EEPROM.read/write/update/get/put`, `EEPROM.length()`). An erased
EEPROM reads 0xFF in every byte: `EEPROM.get` of an `int` then gives -1, of an `unsigned int` 65535, of a float
NaN. EEPROM keeps its content across resets and across tests in the same run, so a test that writes EEPROM
changes what the next test reads; tests that depend on EEPROM should write the bytes they need first.
`EEPROM.update` writes only bytes that differ; each write takes about 3.3 ms.

## Timers used by libraries
`tone()` uses Timer2, `Servo` uses Timer1 and `TimerOne` reprograms Timer1, so `analogWrite()` on pins 9/10 (Timer1)
or 3/11 (Timer2) stops working while those are active. A periodic timer callback can be tested like an ISR: call
the callback function directly instead of waiting for the timer.
