<!-- source: https://raw.githubusercontent.com/arduino/reference-en/master/Language/Functions/Math/max.adoc | license: CC BY-SA 3.0 -->

# max(x, y)

## Description
Calculates the maximum of two numbers.

## Syntax
`max(x, y)`

## Parameters
`x`: the first number. Allowed data types: any data type. +
`y`: the second number. Allowed data types: any data type.

## Returns
The larger of the two parameter values.

## Example Code
The code ensures that sensVal is at least 20.

```cpp
sensVal = max(sensVal, 20); // assigns sensVal to the larger of sensVal or 20
                            // (effectively ensuring that it is at least 20)
```

## Notes and Warnings
Perhaps counter-intuitively, `max()` is often used to constrain the lower end of a variable's range, while `min()` is used to constrain the upper end of the range.

Because of the way the `max()` function is implemented, avoid using other functions inside the brackets, it may lead to incorrect results
```cpp
max(a--, 0);  // avoid this - yields incorrect results

// use this instead:
max(a, 0);
a--;  // keep other math outside the function
```
