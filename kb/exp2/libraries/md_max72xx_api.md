<!-- source: https://github.com/MajicDesigns/MD_MAX72XX (version 2.10.0) | license: see library sources (no LICENSE file) -->

# MD_MAX72XX library: API

Implements functions that allow the MAX72xx (eg, MAX7219) to be used for LED matrices (64 individual LEDs) Allows the programmer to use the LED matrix as a pixel device, displaying graphics elements much like any other pixel addressable display.

## MD_MAX72xx.h

```cpp
/**
\mainpage Arduino LED Matrix Library
The Maxim 72xx LED Controller IC
--------------------------------
The MAX7219/MAX7221 are compact, serial input/output display drivers that
interface microprocessors to 7-segment numeric LED displays of up to 8 digits,
bar-graph displays, or 64 individual LEDs. Included on-chip are a BCD code-B
decoder, multiplex scan circuitry, segment and digit drivers, and an 8x8 static
RAM that stores each digit.

A 4-wire serial interface (SPI) allows the devices to be cascaded, with
communications passed through the first device in the chain to all others. Individual
elements may be addressed and updated without rewriting the entire display.

This library implements functions that allow the MAX72xx to be used
for LED matrices (64 individual LEDs), allowing the programmer to use the LED
matrix as a pixel device, displaying graphics elements much like any other
pixel addressable display.

Topics
------
- \subpage pageHardware
- \subpage pageSoftware
- \subpage pageConnect
- \subpage pageFontUtility
- \subpage pageRevisionHistory

Copyright
---------
Copyright (C) 2012-16 Marco Colli. All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

\page pageRevisionHistory Revision History
Revision History
----------------
Nov 2017 version 2.9.1
- Changed SPI buffer handling and isolation of AVR specific features (eg PROGMEM)
- Added MD_MAX72xx_Message_ESP8266 example
- Minor source file cleanup
- Added Extended ASCII font, vertical rotated font and RobotEyes font in fontbuilder
- Modifed fontbuilder output code for consistency with new code
- Added getFont(), getMaxFontWidth() methods
- Changed example - replaced MD_KeySwitch with new MD_UISwitch library

Nov 2016 version 2.9.0
- Added WordClock example
- Deprecated USE_LIBRARY_SPI as no problems reported with new implementation
- Changed MD_ branding to new MajicDesigns diamond
- Small adjustments to initialisation code

Mar 2016 version 2.8
- Added example _Message_SD and renamed _Message to _Message_Serial
- Added Pacman example
- Added PushWheel example
- Added USE_LIBRARY_SPI to enable library SPI object
- Modifed all examples to conditionally include <SPI.h>
- FontBuilder modified to handle definitions for double height fonts
- New txt2font utility for easier font creattion from a text file
- Revised and ro-organised documentation; expanded section on fonts

April 2015 version 2.7
- Changed to Daft Punk example to run without switch
- Now supporting IDE Library Manager

February 2015 version 2.6
- Improvements to HW_Mapper utility
- Added HW_USE_FC16 for FC-16 display modules
- Added USE_HW_OTHER for user defined hardware configuration
- Fixed incorrect spelling for HW_REV_COLS in transformbuffer() & corresponding bug
- Added PrintText_ML example

February 2015 version 2.5
- Documented process for adding new hardware module type
- Fixed PROGMEM definitions for IDE version 1.5.7 compile error
- Added Daft Punk example code
- Updated HW_Mapper example/utility with built-in instructions
- Minor problems with Parola font setting interaction fixed

April 2014 version 2.4
- Improved reliability of initialization code to remove artifacts
 + Changed order of hardware initialization for SS, _csPin
 + Changed initialisation sequence at begin()
 + Fixed memset bug identified by bperrybap
- Reworked command SPI transmissions for efficiency
- Cleanup up compiler warnings on inline wrapper code functions
- Cleaned up examples begin() - better defined library default values
- Reviewed and tidied up some documentation

March 2014 - version 2.3
- Extensive rework of the font system
 + New font Microsoft Excel VBA based builder tool available
 + Removed USE_FONT_ADJUST and related code - replace by builder tool
 + Fixed width font has been removed from the library. Definition still available in font builder
 + fontype_t definition changed to suit new requirements
- Transform zoning implemented (contiguous subset of services)
 + Transformation functions, control, clear, setRow methods overloaded with range specifier
 + User callback for L/R rotation function syntax added a device parameter
 + New Zones example
- USE_*_HW hardware types now separated out for future flexibility
 + Rework of the library to use new schema for defining hardware characteristics
 + New utility code to map out digits and segments for unknown hardware types
- Rechecked and reworked examples for new library

November 2013 - version 2.2
- Replaced reference to SPI library with inline code to allow for different select lines
- Obsoleted INCLUDE_HARDWARE_SPI conditional compile switch
- Fixed legacy code function name error when USE_FONT_ADJUST switch turned on
- Implemented USE_PAROLA_HW to allow cheaply available matrix modules to be used in ganged mode
- Fixed reversal of bit field for set/get Row/Column functions -> flipped charset data
- Added Eyes example program
- Upgraded and reorganized documentation

June 2013 - version 2.1
- Include the selection of hardware SPI interface (10x speed improvement)
- Tidied up comments

April 2013 - version 2.0
- Major update and rewrite of library code:
- Improved speed and efficiency of code
- Increased level of abstraction in the library for pixel methods
- Increased level of abstraction for character and font methods
- Increased number of functions and added variable sized font
- Changed defines to enumerated types within the scope of the class
- Updated functionality to simplify controlling multiple devices
- Changed text and comments to be aligned to doxygen documentation generation

June 2012 - version 1.0
- Incorporated elements of Arduino LedControl (Eberhard Fahle) and MAX7219 libraries
- Easier functionality for pixel graphics treatment of 8x8 matrices

\page pageSoftware Software Library
The Library
-----------
The library implements functions that allow the MAX72xx to be used
for LED matrices (64 individual LEDs), allowing the programmer to use the LED
matrix as a pixel device, displaying graphics elements much like any other
pixel addressable display.

In this scenario, it is convenient to abstract out the concept of the hardware device
and create a uniform and consistent pixel address space, with the libraries determining
device and device-element address. Similarly, control of the devices should be uniform
and abstracted to a system level.

The library still retains flexibility for device level control, should the developer
require, through the use of overloaded class methods.
___

Conditional Compilation Switches
--------------------------------
The library allows the run time code to be tailored through the use of compilation
switches. The compile options start with USE_ and are documented in the section
related to the main header file MD_MAX72xx.h.

_NOTE_: Compile switches must be edited in the library header file. Arduino header file
'mashing' during compilation makes the setting of these switches from user code
completely unreliable.

\page pageConnect System Connections
Connections to the Arduino Board (SPI interface)
------------------------------------------------
The modules are connected through a 4-wire serial interface (SPI), and devices are cascaded,
with communications passed through the first device in the chain to all others. The Arduino
should be connected to the IN side of the first module in the chain.

The Arduino interface is implemented with either
+ The hardware SPI interface, or
+ 3 arbitrary digital outputs that are passed through to the class constructor.

The AVR hardware SPI interface is fast but fixed to predetermined output pins. The more general
software interface uses the Arduino shiftOut() library function, making it slower but allows the
use of arbitrary digital pins to send the data to the device. Which mode is enabled depends
on the class constructor used.

The Arduino interface is implemented with 3 digital outputs that are passed through to
the class constructor. The digital outputs define the SPI interface as follows:
- DIN (MOSI) - the Data IN signal shifts data into the display module. Data is loaded into
the device's internal 16-bit shift register on CLK's rising edge.
- CLK (SCK) - the CLocK signal that is used to time the data for the device.
- LD (SS) - the interface is active when LoaD signal is LOW. Serial data are loaded into the
device shift register while LOAD is LOW and latched in on the rising edge.

The LD signal is used to select the entire device chain. This allows separate LD
outputs to control multiple displays sharing the same DIN and CLK signals. The
software needs to instantiate a separate object for each display.

The remaining interface pins are for +5V and GND. The power supply must be able to supply
enough current for the number of connected modules.
*/
#ifndef MD_MAX72xx_h
#define MD_MAX72xx_h

#include <Arduino.h>

/**
 * \file
 * \brief Main header file for the MD_MAX72xx library
 */

/**
 \def USE_PAROLA_HW
 Set to 1 (default) to use the Parola hardware modules. The
 software was originally designed to operate with this hardware type.
 */
#define	USE_PAROLA_HW	1

/**
 \def USE_GENERIC_HW
 Set to 1 to use 'generic' hardware modules commonly available, with
 connectors at the top and bottom of the PCB, available from many sources.
 */
#define	USE_GENERIC_HW	0

/**
 \def USE_ICSTATION_HW
 Set to 1 to use ICStation DIY hardware module kits available from
 http://www.icstation.com/product_info.php?products_id=2609#.UxqVJyxWGHs
 This hardware must be set up with the input on the RHS.
 */
#define	USE_ICSTATION_HW	0

/**
 \def USE_FC16_HW
 Set to 1 to use FC16 hardware module kits.
 FC16 modules are similar in format to the ICStation modules but are wired differently.
 Modules are identified by a FC-16 designation on the PCB
  */
#define	USE_FC16_HW	0

/**
 \def USE_OTHER_HW
 Set to 1 to use other hardware not defined above.
 Module 0 (Data In) must be set up on the RHS and the CUSTOM hardware defines
 must be set up in the MD_MAD72xx_lib.h file under for this section, using the HW_Mapper
 utility to work out what the correct values to use are.
 */
#define	USE_OTHER_HW	0

/**
 \def USE_LOCAL_FONT
 Set to 1 (default) to enable local font in this library and enable
 loadChar() and related methods. If the library is just used for
 graphics some FLASH RAM can be saved by not including the code to process
 font data. The font file is stored in PROGMEM.
 */
#define	USE_LOCAL_FONT	1

/**
 \def USE_INDEX_FONT
 Set to 1 to enable font indexing to speed up font lookups - usually disabled.
 This will trade off increased stack RAM usage for lookup speed if enabled.
 When disabled lookups will then become linear searches through PROGMEM.
 Uses ASCII_INDEX_SIZE elements of uint16_t (512 bytes) if enabled. For most
 purposes the increase in speed is not needed.

 USE_LOCAL FONT must be enabled for this option to take effect.
 */
#define	USE_INDEX_FONT	0

// Display parameter constants
// Defined values that are used throughout the library to define physical limits
#define	ROW_SIZE	8		///< The size in pixels of a row in the device LED matrix array
#define COL_SIZE  8		///< The size in pixels of a column in the device LED matrix array
#define	MAX_INTENSITY	0xf	///< The maximum intensity value that can be set for a LED array
#define	MAX_SCANLIMIT	7	  ///< The maximum scan limit value that ca
```

