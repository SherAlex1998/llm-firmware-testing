<!-- source: https://raw.githubusercontent.com/arduino/reference-en/master/Language/Functions/External Interrupts/digitalPinToInterrupt.adoc | license: CC BY-SA 3.0 -->

# digitalPinToInterrupt()

## Description
The `digitalPinToInterrupt()` function takes a pin as an argument, and returns the same pin *if* it can be used as an interrupt. For example, `digitalPinToInterrupt(4)` on an Arduino UNO will not work, as interrupts are only supported on pins 2,3.

See attachInterrupt() for a full list of supported interrupt pins on all boards.

## Syntax
`digitalPinToInterrupt(pin)`

## Parameters
- `pin` - the pin we want to use for an interrupt.

## Returns
- The pin to interrupt (e.g. `2`)+
- If pin is not available for interrupt, returns `-1`.

## Example Code
This example checks if a pin can be used as an interrupt.

```cpp
int pin = 2;

void setup() {
  Serial.begin(9600);
  int checkPin = digitalPinToInterrupt(pin);

  if (checkPin == -1) {
    Serial.println("Not a valid interrupt pin!");
  } else {
    Serial.println("Valid interrupt pin.");
  }
}

void loop() {
}
```
