# Testing time-dependent logic and state machines

## Time budget
`millis()` returns the milliseconds since the board started as an `unsigned long`; it overflows after about
50 days. `delay(ms)` blocks. A test run must finish within 15 seconds of simulated time, and each test should
take well under a second. When a function takes a duration or a count as a parameter (blink times, step delay,
number of cycles), call it with small values in tests.

## Debounce, timeouts and other millis() logic
Code that compares `millis()` with a stored timestamp is hard to test by waiting. Prefer the function variant
that takes the current time as a parameter, for example `update(reading, now_ms)`, and pass synthetic times:
- a change shorter than the threshold must be ignored;
- a change that lasts exactly the threshold, and one that lasts longer, must be accepted;
- repeat with the input going back, to check both transitions.

Use `unsigned long` for times and differences (`now - start`), which stays correct across the `millis()`
overflow. When real time must be measured, allow a tolerance: `ASSERT_NEAR(elapsed, expected, 5)`.

## State machines
Traffic lights, gates and sequences are state machines. Test them by setting a known state, calling the step
function once, and checking the new state and every output pin after each step. Cover:
- every transition in the order the specification gives;
- the wrap-around from the last state back to the first;
- an invalid state or index, which should be rejected or clamped;
- the outputs for each state (for a traffic light: exactly one lamp, or the documented combination, is on).

## Keeping tests small
The Uno has 2 KB of SRAM and the test framework supports at most 16 tests. Avoid large arrays and strings in
tests, and prefer several short tests with one purpose each over one long test.
