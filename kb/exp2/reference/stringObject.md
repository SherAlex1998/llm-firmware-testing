<!-- source: https://raw.githubusercontent.com/arduino/reference-en/master/Language/Variables/Data Types/stringObject.adoc | license: CC BY-SA 3.0 -->

# String()

## Description
Constructs an instance of the String class. There are multiple versions that construct Strings from different data types (i.e. format them as sequences of characters), including:

* a constant string of characters, in double quotes (i.e. a char array)
* a single constant character, in single quotes
* another instance of the String object
* a constant integer or long integer
* a constant integer or long integer, using a specified base
* an integer or long integer variable
* an integer or long integer variable, using a specified base
* a float or double, using a specified decimal places

Constructing a String from a number results in a string that contains the ASCII representation of that number. The default is base ten, so
```cpp
String thisString = String(13);
```
gives you the String "13". You can use other bases, however. For example,

```cpp
String thisString = String(13, HEX);
```

gives you the String "d", which is the hexadecimal representation of the decimal value 13. Or if you prefer binary,

```cpp
String thisString = String(13, BIN);
```

gives you the String "1101", which is the binary representation of 13.

## Syntax
`String(val)` +
`String(val, base)` +
`String(val, decimalPlaces)`

## Parameters
`val`:  a variable to format as a String. Allowed data types: string, char, byte, int, long, unsigned int, unsigned long, float, double. +
`base`: (optional) the base in which to format an integral value. +
`decimalPlaces`: *only if val is float or double*. The desired decimal places.

## Returns
An instance of the String class.

## Example Code
All of the following are valid declarations for Strings.
```cpp
String stringOne = "Hello String";                    // using a constant String
String stringOne = String('a');                       // converting a constant char into a String
String stringTwo = String("This is a string");        // converting a constant string into a String object
String stringOne = String(stringTwo + " with more");  // concatenating two strings
String stringOne = String(13);                        // using a constant integer
String stringOne = String(analogRead(0), DEC);        // using an int and a base
String stringOne = String(45, HEX);                   // using an int and a base (hexadecimal)
String stringOne = String(255, BIN);                  // using an int and a base (binary)
String stringOne = String(millis(), DEC);             // using a long and a base
String stringOne = String(5.698, 3);                  // using a float and the decimal places
```

## Functions

* #LANGUAGE# charAt()
* #LANGUAGE# compareTo()
* #LANGUAGE# concat()
* #LANGUAGE# c_str()
* #LANGUAGE# endsWith()
* #LANGUAGE# equals()
* #LANGUAGE# equalsIgnoreCase()
* #LANGUAGE# getBytes()
* #LANGUAGE# indexOf()
* #LANGUAGE# lastIndexOf()
* #LANGUAGE# length()
* #LANGUAGE# remove()
* #LANGUAGE# replace()
* #LANGUAGE# reserve()
* #LANGUAGE# setCharAt()
* #LANGUAGE# startsWith()
* #LANGUAGE# substring()
* #LANGUAGE# toCharArray()
* #LANGUAGE# toDouble()
* #LANGUAGE# toInt()
* #LANGUAGE# toFloat()
* #LANGUAGE# toLowerCase()
* #LANGUAGE# toUpperCase()
* #LANGUAGE# trim()

## Operators

* #LANGUAGE# [\ (element access)]
* #LANGUAGE# + (concatenation)
* #LANGUAGE# += (append)
* #LANGUAGE# == (comparison)
* #LANGUAGE# > (greater than)
* #LANGUAGE# >= (greater than or equal to)
* #LANGUAGE# < (less than)
* #LANGUAGE# \<= (less than or equal to)
* #LANGUAGE# != (different from)

* #EXAMPLE# https://www.arduino.cc/en/Tutorial/BuiltInExamples#strings[String Tutorials^]
