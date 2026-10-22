<!-- source: https://raw.githubusercontent.com/arduino/reference-en/master/Language/Functions/Math/min.adoc | license: CC BY-SA 3.0 -->

# min(x, y)

## Description
Calculates the minimum of two numbers.

## Syntax
`min(x, y)`

## Parameters
`x`: the first number. Allowed data types: any data type. +
`y`: the second number. Allowed data types: any data type.

## Returns
The smaller of the two numbers.

## Example Code
The code ensures that it never gets above 100.

```cpp
sensVal = min(sensVal, 100);  // assigns sensVal to the smaller of sensVal or 100
                              // ensuring that it never gets above 100.
```

## Notes and Warnings
Perhaps counter-intuitively, `max()` is often used to constrain the lower end of a variable's range, while `min()` is used to constrain the upper end of the range.

Because of the way the `min()` function is implemented, avoid using other functions inside the brackets, it may lead to incorrect results
```cpp
min(a++, 100);  // avoid this - yields incorrect results

min(a, 100);
a++;  // use this instead - keep other math outside the function
```
