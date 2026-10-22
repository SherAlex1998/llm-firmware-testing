<!-- source: https://raw.githubusercontent.com/arduino/reference-en/master/Language/Variables/Constants/integerConstants.adoc | license: CC BY-SA 3.0 -->

# Integer Constants

## Description
Integer constants are numbers that are used directly in a sketch, like 123. By default, these numbers are treated as int but you can change this with the U and L modifiers (see below).

Normally, integer constants are treated as base 10 (decimal) integers, but special notation (formatters) may be used to enter numbers in other bases.

|===
|Base |Example |Formatter |Comment

|10 (decimal)
|123
|none
|

|2 (binary)
|0b1111011
|leading "0b"
|characters 0&1 valid

|8 (octal)
|0173
|leading "0"
|characters 0-7 valid

|16 (hexadecimal)
|0x7B
|leading "0x"
|characters 0-9, A-F, a-f valid
|===

# Decimal (base 10)
This is the common-sense math with which you are acquainted. Constants without other prefixes are assumed to be in decimal format.

## Example  Code:
```cpp
n = 101;  // same as 101 decimal ((1 * 10^2) + (0 * 10^1) + 1)
```

# Binary (base 2)
Only the characters 0 and 1 are valid.

## Example  Code:
```cpp
n = 0b101; // same as 5 decimal ((1 * 2^2) + (0 * 2^1) + 1)
```

# Octal (base 8)
Only the characters 0 through 7 are valid. Octal values are indicated by the prefix "0" (zero).

## Example  Code:
```cpp
n = 0101; // same as 65 decimal ((1 * 8^2) + (0 * 8^1) + 1)
```
It is possible to generate a hard-to-find bug by (unintentionally) including a leading zero before a constant and having the compiler unintentionally interpret your constant as octal.

# Hexadecimal (base 16)
Valid characters are 0 through 9 and letters A through F; A has the value 10, B is 11, up to F, which is 15. Hex values are indicated by the prefix "0x". Note that A-F may be upper (A-F) or lower case (a-f).

## Example  Code:
```cpp
n = 0x101;  // same as 257 decimal ((1 * 16^2) + (0 * 16^1) + 1)
```

## Notes and Warnings
*U & L formatters:*

By default, an integer constant is treated as an int with the attendant limitations in values. To specify an integer constant with another data type, follow it with:

  - a 'u' or 'U' to force the constant into an unsigned data format. Example: 33u
  - a 'l' or 'L' to force the constant into a long data format. Example: 100000L
  - a 'ul' or 'UL' to force the constant into an unsigned long constant. Example: 32767ul
