<!-- source: https://raw.githubusercontent.com/arduino/reference-en/master/Language/Functions/Interrupts/interrupts.adoc | license: CC BY-SA 3.0 -->

# interrupts()

## Description
Re-enables interrupts (after they've been disabled by noInterrupts(). Interrupts allow certain important tasks to happen in the background and are enabled by default. Some functions will not work while interrupts are disabled, and incoming communication may be ignored. Interrupts can slightly disrupt the timing of code, however, and may be disabled for particularly critical sections of code.

## Syntax
`interrupts()`

## Parameters
None

## Returns
Nothing

## Example Code
The code enables Interrupts.

```cpp
void setup() {}

void loop() {
  noInterrupts();
  // critical, time-sensitive code here
  interrupts();
  // other code here
}
```
