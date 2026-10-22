<!-- source: https://github.com/Makuna/Rtc/wiki (version 2.5.0) | license: GNU LESSER GENERAL PUBLIC LICENSE -->

# Rtc by Makuna library: API

A library that makes interfacing DS1302, DS1307, DS3231, DS3234, and PCF8563/BM8563 Real Time Clock modules easy. Includes deep support of module features, including temperature, alarms and memory storage if present.  Tested on esp8266.

## EepromAT24C32.h

```cpp
/*-------------------------------------------------------------------------
RTC library

Written by Michael C. Miller.

I invest time and resources providing this open source code,
please support me by dontating (see https://github.com/Makuna/Rtc)

-------------------------------------------------------------------------
This file is part of the Makuna/Rtc library.

Rtc is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

Rtc is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with Rtc.  If not, see
<http://www.gnu.org/licenses/>.
-------------------------------------------------------------------------*/

#include <Arduino.h>

#pragma once

//I2C Slave Address  
const uint8_t AT24C32_ADDRESS = 0x50; // 0b0 1010 A2 A1 A0

template<class T_WIRE_METHOD> class EepromAt24c32
{
public:
    EepromAt24c32(T_WIRE_METHOD& wire, uint8_t addressBits = 0b000) :
        _address(AT24C32_ADDRESS | (addressBits & 0b00000111)),
        _wire(wire),
        _lastError(Rtc_Wire_Error_None)
    {
    }

    void Begin()
    {
        _wire.begin();
    }

    void Begin(int sda, int scl)
    {
        _wire.begin(sda, scl);
    }

    uint8_t LastError()
    {
        return _lastError;
    }

    void SetMemory(uint16_t memoryAddress, uint8_t value)
    {
        SetMemory(memoryAddress, &value, 1);
    }

    uint8_t GetMemory(uint16_t memoryAddress)
    {
        uint8_t value;

        GetMemory(memoryAddress, &value, 1);
       
        return value;
    }

    // note: this method will write within a single page of eeprom.
    // Pages are 32 bytes (5 bits), so writing past a page boundary will
    // just wrap within the page of the starting memory address.  
    // 
    // xxxppppp pppaaaaa => p = page #, a = address within the page
    //
    // NOTE: hardware WIRE libraries often have a limit of a 32 byte send buffer.  The 
    // effect of this is that only 30 bytes can be sent, 2 bytes for the address to write to,
    // and then 30 bytes of the actual data. 
    uint8_t SetMemory(uint16_t memoryAddress, const uint8_t* pValue, uint8_t countBytes)
    {
        uint8_t countWritten = 0;

        beginTransmission(memoryAddress);

        while (countBytes > 0)
        {
            _wire.write(*pValue++);
            delay(10); // per spec, memory writes

            countBytes--;
            countWritten++;
        }

        _lastError = _wire.endTransmission();
        
        return countWritten;
    }

    // reading data does not wrap within pages, but due to only using
    // 12 (32K) or 13 (64K) bits are used, they will wrap within the memory limits
    // of the installed EEPROM
    //
    // NOTE: hardware WIRE libraries may have a limit of a 32 byte recieve buffer.  The 
    // effect of this is that only 32 bytes can be read at one time.
    uint8_t GetMemory(uint16_t memoryAddress, uint8_t* pValue, uint8_t countBytes)
    {
        // set address to read from
        beginTransmission(memoryAddress);
        _lastError = _wire.endTransmission();

        if (_lastError != 0)
        {
            return 0;
        }

        // read the data
        uint8_t countRead = 0;

        countRead = _wire.requestFrom(_address, countBytes);
        countBytes = countRead;

        while (countBytes-- > 0)
        {
            *pValue++ = _wire.read();
        }

        return countRead;
    }

private:
    const uint8_t _address;
    
    T_WIRE_METHOD& _wire;
    uint8_t _lastError;
    
    void beginTransmission(uint16_t memoryAddress)
    {
        _wire.beginTransmission(_address);
        _wire.write(memoryAddress >> 8);
        _wire.write(memoryAddress & 0xFf);
    }
};
```

## RtcAlarmManager.h

```cpp
/*-------------------------------------------------------------------------
RTC library

Written by Michael C. Miller.

I invest time and resources providing this open source code,
please support me by dontating (see https://github.com/Makuna/Rtc)

-------------------------------------------------------------------------
This file is part of the Makuna/Rtc library.

Rtc is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

Rtc is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with Rtc.  If not, see
<http://www.gnu.org/licenses/>.
-------------------------------------------------------------------------*/

#pragma once

#include <Arduino.h>
#include "RtcUtility.h"
#include "RtcDateTime.h"

#if defined(RTC_NO_STL)

typedef void(*RtcAlarmCallback)(void* context, uint8_t id, const RtcDateTime& alarm);

#else

#undef max
#undef min
#include <functional>
typedef std::function<void(void* context, uint8_t id, const RtcDateTime& alarm)> RtcAlarmCallback;

#endif

enum AlarmPeriod
{
    AlarmPeriod_Expired,
    AlarmPeriod_SingleFire,
    AlarmPeriod_Yearly, 
    AlarmPeriod_Monthly, 
    AlarmPeriod_Monthly_LastDay,
    AlarmPeriod_Weekly, 
    AlarmPeriod_Daily,
    AlarmPeriod_Hourly,
    // the below values are for internal use only
    AlarmPeriod_Yearly_Feb29th, // last of month in Feb if days less than and not a leap year, 
    AlarmPeriod_Monthly_29th, // last of month if days less than, 
    AlarmPeriod_Monthly_30th, // otherwise the day of month matching,
    AlarmPeriod_Monthly_31st, // this will be set internally, just use monthly
    AlarmPeriod_StartOfSpecifics = 60 // anything over this is considered a specific time in seconds
};

enum AlarmAddError
{
    AlarmAddError_PeriodInvalid = -4,
    AlarmAddError_TimePast,
    AlarmAddError_TimeInvalid,
    AlarmAddError_CountExceeded,
};

class RtcAlarmManager
{
public:
    // This class is not meant to be copied nor duplicated
    // 
    // no copy/move constructor
    RtcAlarmManager(const RtcAlarmManager& other) = delete;
    RtcAlarmManager(RtcAlarmManager&& other) noexcept = delete;
    // no copy/move assignment
    RtcAlarmManager& operator=(const RtcAlarmManager& other) = delete;
    RtcAlarmManager& operator=(RtcAlarmManager&& other) noexcept = delete;
    void operator=(RtcAlarmManager& other) = delete;
    // no empty constructor with assignment
    // RtcAlarmManager v = RtcAlarmManager(); // may consider it a functor call
    //
    void operator()() = delete;

    // construct
    RtcAlarmManager() :
        _alarms(nullptr),
        _alarmsCount(0),
        _msLast(0),
        _seconds(0)
    {
    }

    ~RtcAlarmManager()
    {
        Serial.print("~RtcAlarmManager (");
        Serial.print((uint32_t)_alarms, HEX);
        Serial.println(")");

        delete[] _alarms;
    }

    void Begin(uint8_t count)
    {
        if (count > _alarmsCount)
        {
            _alarmsCount = count;
            delete [] _alarms;
            _alarms = new Alarm[_alarmsCount];

            Serial.print("RtcAlarmManager (");
            Serial.print((uint32_t)_alarms, HEX);
            Serial.println(")");

            _msLast = millis();
            _seconds = 0;
        }
    }

    // Expand the number of alarms the manager can handle
    // You can never compress, so use this rarely
    //
    void Expand(uint8_t count)
    {
        if (count > _alarmsCount)
        {
            Alarm* alarmsOld = _alarms;
            _alarms = nullptr;
            Alarm* alarms = new Alarm[count];

            // copy existing
            for (uint8_t alarm = 0; alarm < _alarmsCount; alarm++)
            {
                alarms[alarm] = alarmsOld[alarm];
            }

            delete [] alarmsOld;
            _alarms = alarms;
            _alarmsCount = count;
        }
    }

    // Sync the time to the external trusted source, like
    // a RTC module
    // Do this at regular intervals as the internal CPU timing
    // is not very accurate
    int32_t Sync(const RtcDateTime& now)
    {
        uint32_t msNow = millis();
        uint32_t secondsNow = now.TotalSeconds();
        // calc an updated seconds for old information
        uint32_t secondsOld = _seconds + (msNow - _msLast) / 1000;
        // set new seconds and start tracking the millis
        _msLast = msNow;
        _seconds = secondsNow;
        // return the delta from new seconds from old seconds
        return (secondsNow - secondsOld);
    }

    // retrieve what the current time the AlarmManager thinks it is
    // due to inaccuracy of the CPU timing this may not be exact,
    // but it is good enough for most timing needs
    // regular use of Sync() will improve this
    RtcDateTime Now() const
    {
        uint32_t msNow = millis();
        uint32_t secondsNow = _seconds + (msNow - _msLast) / 1000;
        return RtcDateTime(secondsNow);
    }

    // add an alarm
    // when - the date and time to start triggering alarms
    // period - the type of alarm, does it repeat and how often, see AlarmPeriod enum
    // return - if positive, the id of the Alarm, otherwise see AlarmAddError
    int8_t AddAlarm(const RtcDateTime& when,
        uint32_t period)
    {
        if (!when.IsValid())
        {
            return AlarmAddError_TimeInvalid;
        }
        if (period > AlarmPeriod_Monthly_31st &&
            period < AlarmPeriod_StartOfSpecifics)
        {
            return AlarmAddError_PeriodInvalid;
        }

        int8_t result = AlarmAddError_TimeInvalid;
        uint32_t seconds = when.TotalSeconds();

        if (period == AlarmPeriod_Monthly_LastDay)
        {
            period = AlarmPeriod_Monthly_31st;
            // adjust given when to last day of its set month
            uint8_t daysInMonth = RtcDateTime::DaysInMonth(when.Year(), when.Month());
            if (when.Day() < daysInMonth)
            {
                RtcDateTime temp(when.Year(),
                    when.Month(),
                    daysInMonth,
                    when.Hour(),
                    when.Minute(),
                    when.Second());
                seconds = temp.TotalSeconds();
            }
        }
        else if (period == AlarmPeriod_Monthly ||
            (period >= AlarmPeriod_Monthly_29th && period <= AlarmPeriod_Monthly_31st))
        {
            period = AlarmPeriod_Monthly;
            // adjust alarm period to store target day of month
            // for when months have less days than the target
            // it will trigger on the last day of the month but
            // retain and trigger on specific day of month when
            // available
            if (when.Day() == 29)
            {
                period = AlarmPeriod_Monthly_29th;
            }
            else if (when.Day() == 30)
            {
                period = AlarmPeriod_Monthly_30th;
            }
            else if (when.Day() == 31)
            {
                period = AlarmPeriod_Monthly_31st;
            }
        }
        else if (period == AlarmPeriod_Yearly)
        {
            if (when.Day() == 29 && when.Month() == 2)
            {
                // adjust alarm period to store target day of month
                // for when Feb 29th is target but following year isn't 
                // a leap year it will trigger on the last day of Feb but
                // retain and trigger on specific day of month when
                // available
                period = AlarmPeriod_Yearly_Feb29th;
            }
        }

        Alarm alarm(seconds, period);

        // if the alarm was added that was already in the past,
        // we increment the when to the next repeat
        // for non-repeatable alarms this may expire them
        if (seconds <= _seconds)
        {
            alarm.IncrementWhen();
        }

        if (alarm.Period == AlarmPeriod_Expired)
        {
            result = AlarmAddError_TimePast;
        }
        else
        {
            result = AlarmAddError_CountExceeded;

            for (uint8_t id = 0; id < _alarmsCount; id++)
            {
                if (_alarms[id].Period == AlarmPeriod_Expired)
                {
                    _alarms[id] = alarm;
                    result = id;
                    break;
                }
            }
        }
        
        return result;
    }

    // remove an existing alarm 
    // id - previously returned id from AddAlarm()
    void RemoveAlarm(uint8_t id)
    {
        if (id < _alarmsCount)
        {
            _alarms[id].Period = AlarmPeriod_Expired;
        }
    }

    // check if the alarm is still active 
    // id - previously returned id from AddAlarm()
    bool IsAlarmActive(uint8_t id)
    {
        if (id < _alarmsCount)
        {
            return (_alarms[id].Period != AlarmPeriod_Expired);
        }
        return false;
    }

    // process all the alarms which can trigger callbacks
    // call at regular intervals, if you need seconds accuracy, call
    // every second.  
    // There is little need to call this faster than a few
    // times per second but it doesn't hurt anything
    void ProcessAlarms(RtcAlarmCallback callback, void* context)
    {
        uint32_t msNow = millis();
        uint32_t msDelta = (msNow - _msLast);

        if (msDelta > 1000)
        {
            // update seconds based on passed time using millis()
            _seconds += msDelta / 1000;
            _msLast = msNow - (msDelta % 1000); // retain fractional second
            
            // used a local seconds in case a callback changes it
            uint32_t seconds = _seconds; 

            for (uint8_t id = 0; id < _alarmsCount; id++)
            {
                if (_alarms[id].Period != AlarmPeriod_Expired)
                {
                    if (_alarms[id].When <= seconds)
                    {
                        RtcDateTime alarm(_alarms[id].When);

                        if (_alarms[id].Period == AlarmPeriod_SingleFire)
                        {
                            // remove from list
                            _alarms[id].Period = AlarmPeriod_Expired;
                        }
                        else
                        {
                            _alarms[id].IncrementWhen();
                        }

                        // make callback
                        callback(context, id, alarm);
                    }
                }
            }
        }
    }

protected:
    struct Alarm
    {
        uint32_t When; // seconds from RtcDateTime.TotalSeconds()
        uint32_t Period;  
        
        Alarm(uint32_t when = 0, uint32_t period = AlarmPeriod_Expired) :
            When(when),
            Period(period)
        {
        }

        void IncrementWhen()
        {
            switch (Period)
            {
            case AlarmPeriod_Expired:
                break;

            case AlarmPeriod_SingleFire:
                Period = AlarmPeriod_Expired;
                break;

            case AlarmPeriod_Yearly:
                {
                    RtcDateTime temp(When);
                    RtcDateTime next(temp.Year() + 1,
                        temp.Month(),
                        temp.Day(),
                        temp.Hour(),
                        temp.Minute(),
                        temp.Second());
                    When = next.TotalSeconds();
                }
                break;

            case AlarmPeriod_Yearly_Feb29th:
                {
                    RtcDateTime temp(When);
                    uint16_t year = temp.Year() + 1;
```

## RtcDS1302.h

```cpp
/*-------------------------------------------------------------------------
RTC library

Written by Michael C. Miller.

I invest time and resources providing this open source code,
please support me by dontating (see https://github.com/Makuna/Rtc)

-------------------------------------------------------------------------
This file is part of the Makuna/Rtc library.

Rtc is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

Rtc is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with Rtc.  If not, see
<http://www.gnu.org/licenses/>.
-------------------------------------------------------------------------*/

#pragma once

#include <Arduino.h>
#include "RtcUtility.h"
#include "RtcDateTime.h"
#include "ThreeWire.h"


//DS1302 Register Addresses
const uint8_t DS1302_REG_TIMEDATE   = 0x80;
const uint8_t DS1302_REG_TIMEDATE_BURST = 0xBE;
const uint8_t DS1302_REG_TCR    = 0x90;
const uint8_t DS1302_REG_RAM_BURST = 0xFE;
const uint8_t DS1302_REG_RAMSTART   = 0xc0;
const uint8_t DS1302_REG_RAMEND     = 0xfd;
// ram read and write addresses are interleaved
const uint8_t DS1302RamSize = 31;


// DS1302 Trickle Charge Control Register Bits
enum DS1302TcrResistor : uint8_t
{
    DS1302TcrResistor_Disabled = 0,
    DS1302TcrResistor_2KOhm = 0b00000001,
    DS1302TcrResistor_4KOhm = 0b00000010,
    DS1302TcrResistor_8KOhm = 0b00000011,
    DS1302TcrResistor_MASK  = 0b00000011,
};

enum DS1302TcrDiodes : uint8_t
{
    DS1302TcrDiodes_None = 0,
    DS1302TcrDiodes_One      = 0b00000100,
    DS1302TcrDiodes_Two      = 0b00001000,
    DS1302TcrDiodes_Disabled = 0b00001100,
    DS1302TcrDiodes_MASK     = 0b00001100,
};

enum DS1302TcrStatus : uint8_t
{
    DS1302TcrStatus_Enabled  = 0b10100000,
    DS1302TcrStatus_Disabled = 0b01010000,
    DS1302TcrStatus_MASK     = 0b11110000,
};

const uint8_t DS1302Tcr_Disabled = static_cast<uint8_t>(DS1302TcrStatus_Disabled) | 
                                   static_cast<uint8_t>(DS1302TcrDiodes_Disabled) | 
                                   static_cast<uint8_t>(DS1302TcrResistor_Disabled);

// DS1302 Clock Halt Register & Bits
const uint8_t DS1302_REG_CH = 0x80; // bit in the seconds register
const uint8_t DS1302_CH     = 7;

// Write Protect Register & Bits
const uint8_t DS1302_REG_WP = 0x8E; 
const uint8_t DS1302_WP = 7;

template<class T_WIRE_METHOD> class RtcDS1302
{
public:
    RtcDS1302(T_WIRE_METHOD& wire) :
        _wire(wire)
    {
    }

    void Begin()
    {
        _wire.begin();
    }

    bool GetIsWriteProtected()
    {
        uint8_t wp = getReg(DS1302_REG_WP);
        return !!(wp & _BV(DS1302_WP));
    }

    void SetIsWriteProtected(bool isWriteProtected)
    {
        uint8_t wp = getReg(DS1302_REG_WP);
        if (isWriteProtected)
        {
            wp |= _BV(DS1302_WP);
        }
        else
        {
            wp &= ~_BV(DS1302_WP);
        }
        setReg(DS1302_REG_WP, wp);
    }

    bool IsDateTimeValid()
    {
        return GetDateTime().IsValid();
    }

    bool GetIsRunning()
    {
        uint8_t ch = getReg(DS1302_REG_CH);
        return !(ch & _BV(DS1302_CH));
    }

    void SetIsRunning(bool isRunning)
    {
        uint8_t ch = getReg(DS1302_REG_CH);
        if (isRunning)
        {
            ch &= ~_BV(DS1302_CH);
        }
        else
        {
            ch |= _BV(DS1302_CH);
        }
        setReg(DS1302_REG_CH, ch);
    }

    uint8_t GetTrickleChargeSettings()
    {
        uint8_t setting = getReg(DS1302_REG_TCR);
        return setting;
    }

    void SetTrickleChargeSettings(uint8_t setting)
    {
        if ((setting & DS1302TcrResistor_MASK) == DS1302TcrResistor_Disabled) 
        {
            // invalid resistor setting, set to disabled
            setting = DS1302Tcr_Disabled;
        }
        else if ((setting & DS1302TcrDiodes_MASK) == DS1302TcrDiodes_Disabled ||
            (setting & DS1302TcrDiodes_MASK) == DS1302TcrDiodes_None) 
        {
            // invalid diode setting, set to disabled
            setting = DS1302Tcr_Disabled;
        }
        else if ((setting & DS1302TcrStatus_MASK) != DS1302TcrStatus_Enabled) 
        {
            // invalid status setting, set to disabled
            setting = DS1302Tcr_Disabled;
        }

        setReg(DS1302_REG_TCR, setting);
    }

    void SetDateTime(const RtcDateTime& dt)
    {
        // set the date time
        _wire.beginTransmission(DS1302_REG_TIMEDATE_BURST);

        _wire.write(Uint8ToBcd(dt.Second()));
        _wire.write(Uint8ToBcd(dt.Minute()));
        _wire.write(Uint8ToBcd(dt.Hour())); // 24 hour mode only
        _wire.write(Uint8ToBcd(dt.Day()));
        _wire.write(Uint8ToBcd(dt.Month()));

        // RTC Hardware Day of Week is 1-7, 1 = Monday
        // convert our Day of Week to Rtc Day of Week
        uint8_t rtcDow = RtcDateTime::ConvertDowToRtc(dt.DayOfWeek());

        _wire.write(Uint8ToBcd(rtcDow));
        _wire.write(Uint8ToBcd(dt.Year() - 2000));
        _wire.write(0); // no write protect, as all of this is ignored if it is protected

        _wire.endTransmission();
    }

    RtcDateTime GetDateTime()
    {
        _wire.beginTransmission(DS1302_REG_TIMEDATE_BURST | THREEWIRE_READFLAG);

        uint8_t second = BcdToUint8(_wire.read() & 0x7F);
        uint8_t minute = BcdToUint8(_wire.read());
        uint8_t hour = BcdToBin24Hour(_wire.read());
        uint8_t dayOfMonth = BcdToUint8(_wire.read());
        uint8_t month = BcdToUint8(_wire.read());

        _wire.read();  // throwing away day of week as we calculate it

        uint16_t year = BcdToUint8(_wire.read()) + 2000;

        _wire.read();  // throwing away write protect flag

        _wire.endTransmission();

        return RtcDateTime(year, month, dayOfMonth, hour, minute, second);
    }

    void SetMemory(uint8_t memoryAddress, uint8_t value)
    {
        // memory addresses interleaved read and write addresses
        // so we need to calculate the offset
        uint8_t address = memoryAddress * 2 + DS1302_REG_RAMSTART;
        if (address <= DS1302_REG_RAMEND)
        {
            setReg(address, value);
        }
    }

    uint8_t GetMemory(uint8_t memoryAddress)
    {
        uint8_t value = 0;
        // memory addresses interleaved read and write addresses
        // so we need to calculate the offset
        uint8_t address = memoryAddress * 2 + DS1302_REG_RAMSTART;
        if (address <= DS1302_REG_RAMEND)
        {
            value = getReg(address);
        }
        return value;
    }

    uint8_t SetMemory(const uint8_t* pValue, uint8_t countBytes)
    {
        uint8_t countWritten = 0;

        _wire.beginTransmission(DS1302_REG_RAM_BURST);

        while (countBytes > 0 && countWritten < DS1302RamSize)
        {
            _wire.write(*pValue++);
            countBytes--;
            countWritten++;
        }

        _wire.endTransmission();

        return countWritten;
    }

    uint8_t GetMemory(uint8_t* pValue, uint8_t countBytes)
    {
        uint8_t countRead = 0;

        _wire.beginTransmission(DS1302_REG_RAM_BURST | THREEWIRE_READFLAG);

        while (countBytes > 0 && countRead < DS1302RamSize)
        {
            *pValue++ = _wire.read();
            countRead++;
            countBytes--;
        }

        _wire.endTransmission();

        return countRead;
    }

private:
    T_WIRE_METHOD& _wire;

    uint8_t getReg(uint8_t regAddress)
    {
        _wire.beginTransmission(regAddress | THREEWIRE_READFLAG);
        uint8_t regValue = _wire.read();
        _wire.endTransmission();
        return regValue;
    }

    void setReg(uint8_t regAddress, uint8_t regValue)
    {
        _wire.beginTransmission(regAddress);
        _wire.write(regValue);
        _wire.endTransmission();
    }
};
```

## RtcDS1307.h

```cpp
/*-------------------------------------------------------------------------
RTC library

Written by Michael C. Miller.

I invest time and resources providing this open source code,
please support me by dontating (see https://github.com/Makuna/Rtc)

-------------------------------------------------------------------------
This file is part of the Makuna/Rtc library.

Rtc is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

Rtc is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with Rtc.  If not, see
<http://www.gnu.org/licenses/>.
-------------------------------------------------------------------------*/

#pragma once

#include <Arduino.h>
#include "RtcUtility.h"
#include "RtcDateTime.h"

//I2C Slave Address  
const uint8_t DS1307_ADDRESS = 0x68;

//DS1307 Register Addresses
const uint8_t DS1307_REG_TIMEDATE   = 0x00;
const uint8_t DS1307_REG_STATUS     = 0x00;
const uint8_t DS1307_REG_CONTROL    = 0x07;
const uint8_t DS1307_REG_RAMSTART   = 0x08;
const uint8_t DS1307_REG_RAMEND     = 0x3f;
const uint8_t DS1307_REG_RAMSIZE = DS1307_REG_RAMEND - DS1307_REG_RAMSTART;

//DS1307 Register Data Size if not just 1
const size_t DS1307_REG_TIMEDATE_SIZE = 7;

// DS1307 Control Register Bits
const uint8_t DS1307_RS0   = 0;
const uint8_t DS1307_RS1   = 1;
const uint8_t DS1307_SQWE  = 4;
const uint8_t DS1307_OUT   = 7;

// DS1307 Status Register Bits
const uint8_t DS1307_CH       = 7;

enum DS1307SquareWaveOut
{
    DS1307SquareWaveOut_1Hz  =  0b00010000,
    DS1307SquareWaveOut_4kHz =  0b00010001,
    DS1307SquareWaveOut_8kHz =  0b00010010,
    DS1307SquareWaveOut_32kHz = 0b00010011,
    DS1307SquareWaveOut_High =  0b10000000,
    DS1307SquareWaveOut_Low =   0b00000000,
};

template<class T_WIRE_METHOD> class RtcDS1307
{
public:
    RtcDS1307(T_WIRE_METHOD& wire) :
        _wire(wire),
        _lastError(Rtc_Wire_Error_None)
    {
    }

    void Begin()
    {
        _wire.begin();
    }

    void Begin(int sda, int scl)
    {
        _wire.begin(sda, scl);
    }

    uint8_t LastError()
    {
        return _lastError;
    }

    bool IsDateTimeValid()
    {
        return GetIsRunning();
    }

    bool GetIsRunning()
    {
        uint8_t sreg = getReg(DS1307_REG_STATUS);
        return (!(sreg & _BV(DS1307_CH)) && (_lastError == Rtc_Wire_Error_None));
    }

    void SetIsRunning(bool isRunning)
    {
        uint8_t sreg = getReg(DS1307_REG_STATUS);

        if (isRunning)
        {
            sreg &= ~_BV(DS1307_CH);
        }
        else
        {
            sreg |= _BV(DS1307_CH);
        }

        setReg(DS1307_REG_STATUS, sreg);
    }

    void SetDateTime(const RtcDateTime& dt)
    {
        // retain running state
        uint8_t sreg = getReg(DS1307_REG_STATUS) & _BV(DS1307_CH);

        // set the date time
        _wire.beginTransmission(DS1307_ADDRESS);
        _wire.write(DS1307_REG_TIMEDATE);

        _wire.write(Uint8ToBcd(dt.Second()) | sreg);
        _wire.write(Uint8ToBcd(dt.Minute()));
        _wire.write(Uint8ToBcd(dt.Hour())); // 24 hour mode only

        // RTC Hardware Day of Week is 1-7, 1 = Monday
        // convert our Day of Week to Rtc Day of Week
        uint8_t rtcDow = RtcDateTime::ConvertDowToRtc(dt.DayOfWeek());

        _wire.write(Uint8ToBcd(rtcDow)); 
        _wire.write(Uint8ToBcd(dt.Day()));
        _wire.write(Uint8ToBcd(dt.Month()));
        _wire.write(Uint8ToBcd(dt.Year() - 2000));

        _lastError = _wire.endTransmission();
    }

    RtcDateTime GetDateTime()
    {
        _wire.beginTransmission(DS1307_ADDRESS);
        _wire.write(DS1307_REG_TIMEDATE);
        _lastError = _wire.endTransmission();
        if (_lastError != Rtc_Wire_Error_None)
        {
            RtcDateTime(0);
        }

        size_t bytesRead = _wire.requestFrom(DS1307_ADDRESS, DS1307_REG_TIMEDATE_SIZE);
        if (DS1307_REG_TIMEDATE_SIZE != bytesRead)
        {
            _lastError = Rtc_Wire_Error_Unspecific;
            RtcDateTime(0);
        }

        uint8_t second = BcdToUint8(_wire.read() & 0x7F);
        uint8_t minute = BcdToUint8(_wire.read());
        uint8_t hour = BcdToBin24Hour(_wire.read());

        _wire.read();  // throwing away day of week as we calculate it

        uint8_t dayOfMonth = BcdToUint8(_wire.read());
        uint8_t month = BcdToUint8(_wire.read());
        uint16_t year = BcdToUint8(_wire.read()) + 2000;

        return RtcDateTime(year, month, dayOfMonth, hour, minute, second);
    }

    void SetMemory(uint8_t memoryAddress, uint8_t value)
    {
        uint8_t address = memoryAddress + DS1307_REG_RAMSTART;
        if (address <= DS1307_REG_RAMEND)
        {
            setReg(address, value);
        }
    }

    uint8_t GetMemory(uint8_t memoryAddress)
    {
        uint8_t value = 0;
        uint8_t address = memoryAddress + DS1307_REG_RAMSTART;
        if (address <= DS1307_REG_RAMEND)
        {
            value = getReg(address);
        }
        return value;
    }

    uint8_t SetMemory(uint8_t memoryAddress, const uint8_t* pValue, uint8_t countBytes)
    {
        uint8_t address = memoryAddress + DS1307_REG_RAMSTART;
        uint8_t countWritten = 0;
        if (address <= DS1307_REG_RAMEND)
        {
            _wire.beginTransmission(DS1307_ADDRESS);
            _wire.write(address);

            while (countBytes > 0 && address <= DS1307_REG_RAMEND)
            {
                _wire.write(*pValue++);
                address++;
                countBytes--;
                countWritten++;
            }

            _lastError = _wire.endTransmission();
        }
        return countWritten;
    }

    size_t GetMemory(uint8_t memoryAddress, uint8_t* pValue, size_t countBytes)
    {
        uint8_t address = memoryAddress + DS1307_REG_RAMSTART;
        size_t countRead = 0;
        if (address <= DS1307_REG_RAMEND)
        {
            if (countBytes > DS1307_REG_RAMSIZE)
            {
                countBytes = DS1307_REG_RAMSIZE;
            }

            _wire.beginTransmission(DS1307_ADDRESS);
            _wire.write(address);
            _lastError = _wire.endTransmission();
            if (_lastError != Rtc_Wire_Error_None)
            {
                return 0;
            }

            countRead = _wire.requestFrom(DS1307_ADDRESS, countBytes);
            countBytes = countRead;

            while (countBytes-- > 0)
            {
                *pValue++ = _wire.read();
            }
        }

        return countRead;
    }

    void SetSquareWavePin(DS1307SquareWaveOut pinMode)
    {
        setReg(DS1307_REG_CONTROL, pinMode);
    }

private:
    T_WIRE_METHOD& _wire;
    uint8_t _lastError;

    uint8_t getReg(uint8_t regAddress)
    {
        _wire.beginTransmission(DS1307_ADDRESS);
        _wire.write(regAddress);
        _lastError = _wire.endTransmission();
        if (_lastError != Rtc_Wire_Error_None)
        {
            return 0;
        }

        // control register
        size_t bytesRead = _wire.requestFrom(DS1307_ADDRESS, (size_t)1);
        if (1 != bytesRead)
        {
            _lastError = Rtc_Wire_Error_Unspecific;
            return 0;
        }

        uint8_t regValue = _wire.read();
        return regValue;
    }

    void setReg(uint8_t regAddress, uint8_t regValue)
    {
        _wire.beginTransmission(DS1307_ADDRESS);
        _wire.write(regAddress);
        _wire.write(regValue);
        _lastError = _wire.endTransmission();
    }
};
```

## RtcDS3231.h

```cpp
/*-------------------------------------------------------------------------
RTC library

Written by Michael C. Miller.

I invest time and resources providing this open source code,
please support me by dontating (see https://github.com/Makuna/Rtc)

-------------------------------------------------------------------------
This file is part of the Makuna/Rtc library.

Rtc is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

Rtc is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with Rtc.  If not, see
<http://www.gnu.org/licenses/>.
-------------------------------------------------------------------------*/

#pragma once

#include <Arduino.h>
#include "RtcUtility.h"
#include "RtcDateTime.h"
#include "RtcTemperature.h"


//I2C Slave Address  
const uint8_t DS3231_ADDRESS = 0x68;

//DS3231 Register Addresses
const uint8_t DS3231_REG_TIMEDATE  = 0x00;
const uint8_t DS3231_REG_ALARMONE  = 0x07;
const uint8_t DS3231_REG_ALARMTWO  = 0x0B;
                                         
const uint8_t DS3231_REG_CONTROL   = 0x0E;
const uint8_t DS3231_REG_STATUS    = 0x0F;
const uint8_t DS3231_REG_AGING     = 0x10;
                                         
const uint8_t DS3231_REG_TEMP      = 0x11;


//DS3231 Register Data Size if not just 1
const size_t DS3231_REG_TIMEDATE_SIZE = 7;
const size_t DS3231_REG_ALARMONE_SIZE = 4;
const size_t DS3231_REG_ALARMTWO_SIZE = 3;

const size_t DS3231_REG_TEMP_SIZE = 2;

// DS3231 Control Register Bits
const uint8_t DS3231_A1IE  = 0;
const uint8_t DS3231_A2IE  = 1;
const uint8_t DS3231_INTCN = 2;
const uint8_t DS3231_RS1   = 3;
const uint8_t DS3231_RS2   = 4;
const uint8_t DS3231_CONV  = 5;
const uint8_t DS3231_BBSQW = 6;
const uint8_t DS3231_EOSC  = 7;
const uint8_t DS3231_AIEMASK = (_BV(DS3231_A1IE) | _BV(DS3231_A2IE));
const uint8_t DS3231_RSMASK = (_BV(DS3231_RS1) | _BV(DS3231_RS2));

// DS3231 Status Register Bits
const uint8_t DS3231_A1F      = 0;
const uint8_t DS3231_A2F      = 1;
const uint8_t DS3231_BSY      = 2;
const uint8_t DS3231_EN32KHZ  = 3;
const uint8_t DS3231_OSF      = 7;
const uint8_t DS3231_AIFMASK = (_BV(DS3231_A1F) | _BV(DS3231_A2F));


// seconds accuracy
enum DS3231AlarmOneControl
{
    // bit order:  A1M4  DY/DT  A1M3  A1M2  A1M1
    DS3231AlarmOneControl_HoursMinutesSecondsDayOfMonthMatch = 0x00,
    DS3231AlarmOneControl_OncePerSecond = 0x17,
    DS3231AlarmOneControl_SecondsMatch = 0x16,
    DS3231AlarmOneControl_MinutesSecondsMatch = 0x14,
    DS3231AlarmOneControl_HoursMinutesSecondsMatch = 0x10,
    DS3231AlarmOneControl_HoursMinutesSecondsDayOfWeekMatch = 0x08,
};

class DS3231AlarmOne
{
public:
    DS3231AlarmOne( uint8_t dayOf,
            uint8_t hour,
            uint8_t minute,
            uint8_t second,
            DS3231AlarmOneControl controlFlags) :
        _flags(controlFlags),
        _dayOf(dayOf),
        _hour(hour),
        _minute(minute),
        _second(second)
    {
    }

    uint8_t DayOf() const
    {
        return _dayOf;
    }

    uint8_t Hour() const
    {
        return _hour;
    }

    uint8_t Minute() const
    {
        return _minute;
    }

    uint8_t Second() const
    {
        return _second;
    }

    DS3231AlarmOneControl ControlFlags() const
    {
        return _flags;
    }

    bool operator == (const DS3231AlarmOne& other) const
    {
        return (_dayOf == other._dayOf &&
                _hour == other._hour &&
                _minute == other._minute &&
                _second == other._second &&
                _flags == other._flags);
    }

    bool operator != (const DS3231AlarmOne& other) const
    {
        return !(*this == other);
    }

protected:
    DS3231AlarmOneControl _flags;

    uint8_t _dayOf;
    uint8_t _hour;
    uint8_t _minute;
    uint8_t _second;  
};

// minutes accuracy
enum DS3231AlarmTwoControl
{
    // bit order:  A2M4  DY/DT  A2M3  A2M2
    DS3231AlarmTwoControl_HoursMinutesDayOfMonthMatch = 0x00,
    DS3231AlarmTwoControl_OncePerMinute = 0x0b,
    DS3231AlarmTwoControl_MinutesMatch = 0x0a,
    DS3231AlarmTwoControl_HoursMinutesMatch = 0x08,
    DS3231AlarmTwoControl_HoursMinutesDayOfWeekMatch = 0x04,
};

class DS3231AlarmTwo
{
public:
    DS3231AlarmTwo( uint8_t dayOf,
            uint8_t hour,
            uint8_t minute,
            DS3231AlarmTwoControl controlFlags) :
        _flags(controlFlags),
        _dayOf(dayOf),
        _hour(hour),
        _minute(minute)
    {
    }

    uint8_t DayOf() const
    {
        return _dayOf;
    }

    uint8_t Hour() const
    {
        return _hour;
    }

    uint8_t Minute() const
    {
        return _minute;
    }

    DS3231AlarmTwoControl ControlFlags() const
    {
        return _flags;
    }

    bool operator == (const DS3231AlarmTwo& other) const
    {
        return (_dayOf == other._dayOf &&
                _hour == other._hour &&
                _minute == other._minute &&
                _flags == other._flags);
    }

    bool operator != (const DS3231AlarmTwo& other) const
    {
        return !(*this == other);
    }

protected:
    DS3231AlarmTwoControl _flags;

    uint8_t _dayOf;
    uint8_t _hour;
    uint8_t _minute;
};


enum DS3231SquareWaveClock
{
    DS3231SquareWaveClock_1Hz  = 0b00000000,
    DS3231SquareWaveClock_1kHz = 0b00001000,
    DS3231SquareWaveClock_4kHz = 0b00010000,
    DS3231SquareWaveClock_8kHz = 0b00011000,
};

enum DS3231SquareWavePinMode
{
    DS3231SquareWavePin_ModeNone,
    DS3231SquareWavePin_ModeAlarmOne,
    DS3231SquareWavePin_ModeAlarmTwo,
    // note:  the same as DS3231SquareWavePin_ModeAlarmOne | DS3231SquareWavePin_ModeAlarmTwo
    DS3231SquareWavePin_ModeAlarmBoth, 
    DS3231SquareWavePin_ModeClock
};

enum DS3231AlarmFlag
{
    DS3231AlarmFlag_Alarm1 = 0x01,
    DS3231AlarmFlag_Alarm2 = 0x02,
    DS3231AlarmFlag_AlarmBoth = 0x03,
};

template<class T_WIRE_METHOD> class RtcDS3231
{
public:
    RtcDS3231(T_WIRE_METHOD& wire) :
        _wire(wire),
        _lastError(Rtc_Wire_Error_None)
    {
    }

    void Begin()
    {
        _wire.begin();
    }

    void Begin(int sda, int scl)
    {
        _wire.begin(sda, scl);
    }

    uint8_t LastError()
    {
        return _lastError;
    }

    bool IsDateTimeValid()
    {
        uint8_t status = getReg(DS3231_REG_STATUS);
        return (!(status & _BV(DS3231_OSF)) && (_lastError == Rtc_Wire_Error_None));
    }

    bool GetIsRunning()
    {
        uint8_t creg = getReg(DS3231_REG_CONTROL);
        return (!(creg & _BV(DS3231_EOSC)) && (_lastError == Rtc_Wire_Error_None));
    }

    void SetIsRunning(bool isRunning)
    {
        uint8_t creg = getReg(DS3231_REG_CONTROL);
        if (isRunning)
        {
            creg &= ~_BV(DS3231_EOSC);
        }
        else
        {
            creg |= _BV(DS3231_EOSC);
        }
        setReg(DS3231_REG_CONTROL, creg);
    }

    void SetDateTime(const RtcDateTime& dt)
    {
        // clear the invalid flag
        uint8_t status = getReg(DS3231_REG_STATUS);
        status &= ~_BV(DS3231_OSF); // clear the flag
        setReg(DS3231_REG_STATUS, status);

        // set the date time
        _wire.beginTransmission(DS3231_ADDRESS);
        _wire.write(DS3231_REG_TIMEDATE);

        _wire.write(Uint8ToBcd(dt.Second()));
        _wire.write(Uint8ToBcd(dt.Minute()));
        _wire.write(Uint8ToBcd(dt.Hour())); // 24 hour mode only

        uint8_t year = dt.Year() - 2000;
        uint8_t centuryFlag = 0;

        if (year >= 100)
        {
            year -= 100;
            centuryFlag = _BV(7);
        }

        // RTC Hardware Day of Week is 1-7, 1 = Monday
        // convert our Day of Week to Rtc Day of Week
        uint8_t rtcDow = RtcDateTime::ConvertDowToRtc(dt.DayOfWeek());

        _wire.write(Uint8ToBcd(rtcDow));

        _wire.write(Uint8ToBcd(dt.Day()));
        _wire.write(Uint8ToBcd(dt.Month()) | centuryFlag);
        _wire.write(Uint8ToBcd(year));

        _lastError = _wire.endTransmission();
    }

    RtcDateTime GetDateTime()
    {
        _wire.beginTransmission(DS3231_ADDRESS);
        _wire.write(DS3231_REG_TIMEDATE);
        _lastError = _wire.endTransmission();
        if (_lastError != Rtc_Wire_Error_None)
        {
            return RtcDateTime(0);
        }

        size_t bytesRead = _wire.requestFrom(DS3231_ADDRESS, DS3231_REG_TIMEDATE_SIZE);
        if (DS3231_REG_TIMEDATE_SIZE != bytesRead)
        {
            _lastError = Rtc_Wire_Error_Unspecific;
            return RtcDateTime(0);
        }

        uint8_t second = BcdToUint8(_wire.read() & 0x7F);
        uint8_t minute = BcdToUint8(_wire.read());
        uint8_t hour = BcdToBin24Hour(_wire.read());

        _wire.read();  // throwing away day of week as we calculate it

        uint8_t dayOfMonth = BcdToUint8(_wire.read());
        uint8_t monthRaw = _wire.read();
        uint16_t year = BcdToUint8(_wire.read()) + 2000;

        if (monthRaw & _BV(7)) // century wrap flag
        {
            year += 100;
        }
        uint8_t month = BcdToUint8(monthRaw & 0x7f);


        return RtcDateTime(year, month, dayOfMonth, hour, minute, second);
    }

    RtcTemperature GetTemperature()
    {
        _wire.beginTransmission(DS3231_ADDRESS);
        _wire.write(DS3231_REG_TEMP);
        _lastError = _wire.endTransmission();
        if (_lastError != Rtc_Wire_Error_None)
        {
            return RtcTemperature(0);
        }

        // Temperature is represented as a 10-bit code with a resolution
        // of 1/4th °C and is accessable as a signed 16-bit integer at
        // locations 11h and 12h.
        //
        //       |         r11h          | DP |         r12h         |
        // Bit:   15 14 13 12 11 10  9  8   .  7  6  5  4  3  2  1  0  -1 -2
        //         s  i  i  i  i  i  i  i   .  f  f  0  0  0  0  0  0
        //
        // As it takes (8) right-shifts to register the decimal point (DP) to
        // the right of the 0th bit, the overall word scaling equals 256.
        //
        // For example, at +/- 25.25°C, concatenated registers <r11h:r12h> =
        // 256 * (+/- 25+(1/4)) = +/- 6464, or 1940h / E6C0h.

        size_t bytesRead = _wire.requestFrom(DS3231_ADDRESS, DS3231_REG_TEMP_SIZE);
        if (DS3231_REG_TEMP_SIZE != bytesRead)
        {
            _lastError = Rtc_Wire_Error_Unspecific;
            return RtcTemperature(0);
        }

        int8_t  r11h = _wire.read();                  // MS byte, signed temperature
        return RtcTemperature( r11h, _wire.read() );  // LS byte is r12h
    }

    void Enable32kHzPin(bool enable)
    {
        uint8_t sreg = getReg(DS3231_REG_STATUS);

        if (enable == true)
        {
            sreg |= _BV(DS3231_EN32KHZ);
        }
        else
        {
            sreg &= ~_BV(DS3231_EN32KHZ);
        }

        setReg(DS3231_REG_STATUS, sreg);
    }

    void SetSquareWavePin(DS3231SquareWavePinMode pinMode, bool enableWhileInBatteryBackup = true)
    {
        uint8_t creg = getReg(DS3231_REG_CONTROL);

        // clear all relevant bits to a known "off" state
        creg &= ~(DS3231_AIEMASK | _BV(DS3231_BBSQW));
        creg |= _BV(DS3231_INTCN);  // set INTCN to disables clock SQW

        if (pinMode != DS3231SquareWavePin_ModeNone)
        {
            if (pinMode == DS3231SquareWavePin_ModeClock)
            {
                creg &= ~_BV(DS3231_INTCN); // clear INTCN to enable clock SQW 
            }
            else
            {
                if (pinMode & DS3231SquareWavePin_ModeAlarmOne)
                {
                    creg |= _BV(DS3231_A1IE);
                }
                if (pinMode & DS3231SquareWavePin_ModeAl
```

## RtcDS3232.h

```cpp
/*-------------------------------------------------------------------------
RTC library

Written by Michael C. Miller.

I invest time and resources providing this open source code,
please support me by dontating (see https://github.com/Makuna/Rtc)

-------------------------------------------------------------------------
This file is part of the Makuna/Rtc library.

Rtc is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

Rtc is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with Rtc.  If not, see
<http://www.gnu.org/licenses/>.
-------------------------------------------------------------------------*/

#pragma once

#include "RtcDS3231.h"

// DS3232 is the same as the DS3231 except it has SRAM
//
// 
//I2C Slave Address  
const uint8_t DS3232_ADDRESS = DS3231_ADDRESS;

const uint8_t DS3232_REG_SRAMFIRST = 0x14;
const uint8_t DS3232_REG_SRAMLAST = 0xff;

const uint8_t DS3232_REG_SRAMSIZE = DS3232_REG_SRAMLAST - DS3232_REG_SRAMFIRST + 1;

template<class T_WIRE_METHOD> class RtcDS3232 : 
    public RtcDS3231<T_WIRE_METHOD>
{
public:
    RtcDS3232(T_WIRE_METHOD& wire) :
        RtcDS3231<T_WIRE_METHOD>( wire )
    {
    }

    void SetMemory(uint8_t memoryAddress, uint8_t value)
    {
        uint8_t address = memoryAddress + DS3232_REG_SRAMFIRST;
        if (address <= DS3232_REG_SRAMLAST)
        {
            this->setReg(address, value);
        }
    }

    uint8_t GetMemory(uint8_t memoryAddress)
    {
        uint8_t value = 0;
        uint8_t address = memoryAddress + DS3232_REG_SRAMFIRST;
        if (address <= DS3232_REG_SRAMLAST)
        {
            value = this->getReg(address);
        }
        return value;
    }

    uint8_t SetMemory(uint8_t memoryAddress, const uint8_t* pValue, uint8_t countBytes)
    {
        uint8_t address = memoryAddress + DS3232_REG_SRAMFIRST;
        uint8_t countWritten = 0;
        if (address <= DS3232_REG_SRAMLAST)
        {
            this->_wire.beginTransmission(DS3232_ADDRESS);
            this->_wire.write(address);

            while (countBytes > 0 && address <= DS3232_REG_SRAMLAST)
            {
                this->_wire.write(*pValue++);
                address++;
                countBytes--;
                countWritten++;
            }

            this->_lastError = this->_wire.endTransmission();
        }
        return countWritten;
    }

    size_t GetMemory(uint8_t memoryAddress, uint8_t* pValue, size_t countBytes)
    {
        uint8_t address = memoryAddress + DS3232_REG_SRAMFIRST;
        size_t countRead = 0;
        if (address <= DS3232_REG_SRAMLAST)
        {
            if (countBytes > DS3232_REG_SRAMSIZE)
            {
                countBytes = DS3232_REG_SRAMSIZE;
            }

            this->_wire.beginTransmission(DS3232_ADDRESS);
            this->_wire.write(address);
            this->_lastError = this->_wire.endTransmission();
            if (this->_lastError != Rtc_Wire_Error_None)
            {
                return 0;
            }

            countRead = this->_wire.requestFrom(DS3232_ADDRESS, countBytes);
            countBytes = countRead;

            while (countBytes-- > 0)
            {
                *pValue++ = this->_wire.read();
            }
        }

        return countRead;
    }

};
```

## RtcDS3234.h

```cpp
/*-------------------------------------------------------------------------
RTC library

Written by Michael C. Miller.

I invest time and resources providing this open source code,
please support me by dontating (see https://github.com/Makuna/Rtc)

-------------------------------------------------------------------------
This file is part of the Makuna/Rtc library.

Rtc is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

Rtc is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with Rtc.  If not, see
<http://www.gnu.org/licenses/>.
-------------------------------------------------------------------------*/

#pragma once

#include <Arduino.h>
#include <SPI.h>
#include "RtcUtility.h"
#include "RtcDateTime.h"
#include "RtcTemperature.h"



//DS3234 Register Addresses
const uint8_t DS3234_REG_WRITE_FLAG = 0x80;

const uint8_t DS3234_REG_TIMEDATE  = 0x00;

const uint8_t DS3234_REG_ALARMONE  = 0x07;
const uint8_t DS3234_REG_ALARMTWO  = 0x0B;
                                         
const uint8_t DS3234_REG_CONTROL   = 0x0E;
const uint8_t DS3234_REG_STATUS    = 0x0F;
const uint8_t DS3234_REG_AGING     = 0x10;
                                         
const uint8_t DS3234_REG_TEMP      = 0x11;

const uint8_t DS3234_REG_RAM_ADDRESS = 0x18;
const uint8_t DS3234_REG_RAM_DATA = 0x19;

const uint8_t DS3234_RAMSTART = 0x00;
const uint8_t DS3234_RAMEND = 0xFF;
const uint8_t DS3234_RAMSIZE = DS3234_RAMEND - DS3234_RAMSTART;

// DS3234 Control Register Bits
const uint8_t DS3234_A1IE  = 0;
const uint8_t DS3234_A2IE  = 1;
const uint8_t DS3234_INTCN = 2;
const uint8_t DS3234_RS1   = 3;
const uint8_t DS3234_RS2   = 4;
const uint8_t DS3234_CONV  = 5;
const uint8_t DS3234_BBSQW = 6;
const uint8_t DS3234_EOSC  = 7;
const uint8_t DS3234_AIEMASK = (_BV(DS3234_A1IE) | _BV(DS3234_A2IE));
const uint8_t DS3234_RSMASK = (_BV(DS3234_RS1) | _BV(DS3234_RS2));

// DS3234 Status Register Bits
const uint8_t DS3234_A1F      = 0;
const uint8_t DS3234_A2F      = 1;
const uint8_t DS3234_BSY      = 2;
const uint8_t DS3234_EN32KHZ  = 3;
const uint8_t DS3234_CRATE0   = 4;
const uint8_t DS3234_CRATE1   = 5;
const uint8_t DS3234_BB32KHZ  = 6;
const uint8_t DS3234_OSF      = 7;
const uint8_t DS3234_AIFMASK = (_BV(DS3234_A1F) | _BV(DS3234_A2F));
const uint8_t DS3234_CRATEMASK = (_BV(DS3234_CRATE0) | _BV(DS3234_CRATE1));

// seconds accuracy
enum DS3234AlarmOneControl
{
    // bit order:  A1M4  DY/DT  A1M3  A1M2  A1M1
    DS3234AlarmOneControl_HoursMinutesSecondsDayOfMonthMatch = 0x00,
    DS3234AlarmOneControl_OncePerSecond = 0x17,
    DS3234AlarmOneControl_SecondsMatch = 0x16,
    DS3234AlarmOneControl_MinutesSecondsMatch = 0x14,
    DS3234AlarmOneControl_HoursMinutesSecondsMatch = 0x10,
    DS3234AlarmOneControl_HoursMinutesSecondsDayOfWeekMatch = 0x08,
};

class DS3234AlarmOne
{
public:
    DS3234AlarmOne( uint8_t dayOf,
            uint8_t hour,
            uint8_t minute,
            uint8_t second,
            DS3234AlarmOneControl controlFlags) :
        _flags(controlFlags),
        _dayOf(dayOf),
        _hour(hour),
        _minute(minute),
        _second(second)
    {
    }

    uint8_t DayOf() const
    {
        return _dayOf;
    }

    uint8_t Hour() const
    {
        return _hour;
    }

    uint8_t Minute() const
    {
        return _minute;
    }

    uint8_t Second() const
    {
        return _second;
    }

    DS3234AlarmOneControl ControlFlags() const
    {
        return _flags;
    }

    bool operator == (const DS3234AlarmOne& other) const
    {
        return (_dayOf == other._dayOf &&
                _hour == other._hour &&
                _minute == other._minute &&
                _second == other._second &&
                _flags == other._flags);
    }

    bool operator != (const DS3234AlarmOne& other) const
    {
        return !(*this == other);
    }

protected:
    DS3234AlarmOneControl _flags;

    uint8_t _dayOf;
    uint8_t _hour;
    uint8_t _minute;
    uint8_t _second;  
};

// minutes accuracy
enum DS3234AlarmTwoControl
{
    // bit order:  A2M4  DY/DT  A2M3  A2M2
    DS3234AlarmTwoControl_HoursMinutesDayOfMonthMatch = 0x00,
    DS3234AlarmTwoControl_OncePerMinute = 0x0b,
    DS3234AlarmTwoControl_MinutesMatch = 0x0a,
    DS3234AlarmTwoControl_HoursMinutesMatch = 0x08,
    DS3234AlarmTwoControl_HoursMinutesDayOfWeekMatch = 0x04,
};

class DS3234AlarmTwo
{
public:
    DS3234AlarmTwo( uint8_t dayOf,
            uint8_t hour,
            uint8_t minute,
            DS3234AlarmTwoControl controlFlags) :
        _flags(controlFlags),
        _dayOf(dayOf),
        _hour(hour),
        _minute(minute)
    {
    }

    uint8_t DayOf() const
    {
        return _dayOf;
    }

    uint8_t Hour() const
    {
        return _hour;
    }

    uint8_t Minute() const
    {
        return _minute;
    }

    DS3234AlarmTwoControl ControlFlags() const
    {
        return _flags;
    }

    bool operator == (const DS3234AlarmTwo& other) const
    {
        return (_dayOf == other._dayOf &&
                _hour == other._hour &&
                _minute == other._minute &&
                _flags == other._flags);
    }

    bool operator != (const DS3234AlarmTwo& other) const
    {
        return !(*this == other);
    }

protected:
    DS3234AlarmTwoControl _flags;

    uint8_t _dayOf;
    uint8_t _hour;
    uint8_t _minute;
};


enum DS3234SquareWaveClock
{
    DS3234SquareWaveClock_1Hz  = 0b00000000,
    DS3234SquareWaveClock_1kHz = 0b00001000,
    DS3234SquareWaveClock_4kHz = 0b00010000,
    DS3234SquareWaveClock_8kHz = 0b00011000,
};

enum DS3234SquareWavePinMode
{
    DS3234SquareWavePin_ModeNone,
    DS3234SquareWavePin_ModeBatteryBackup,
    DS3234SquareWavePin_ModeClock,
    DS3234SquareWavePin_ModeAlarmOne,
    DS3234SquareWavePin_ModeAlarmTwo,
    DS3234SquareWavePin_ModeAlarmBoth
};

enum DS3234TempCompensationRate
{
    DS3234TempCompensationRate_64Seconds,
    DS3234TempCompensationRate_128Seconds,
    DS3234TempCompensationRate_256Seconds,
    DS3234TempCompensationRate_512Seconds,
};

enum DS3234AlarmFlag
{
    DS3234AlarmFlag_Alarm1 = 0x01,
    DS3234AlarmFlag_Alarm2 = 0x02,
    DS3234AlarmFlag_AlarmBoth = 0x03,
};

const SPISettings c_Ds3234SpiSettings(4000000, MSBFIRST, SPI_MODE3); // CPHA must be used, so mode 1 or mode 3 are valid

template<class T_SPI_METHOD> class RtcDS3234
{
public:
    RtcDS3234(T_SPI_METHOD& spi, uint8_t csPin) :
        _spi(spi),
        _csPin(csPin)
    {
    }

    void Begin()
    {
        UnselectChip();
        pinMode(_csPin, OUTPUT);
    }

    bool IsDateTimeValid()
    {
        uint8_t status = getReg(DS3234_REG_STATUS);
        return !(status & _BV(DS3234_OSF));
    }

    bool GetIsRunning()
    {
        uint8_t creg = getReg(DS3234_REG_CONTROL);
        return !(creg & _BV(DS3234_EOSC));
    }

    void SetIsRunning(bool isRunning)
    {
        uint8_t creg = getReg(DS3234_REG_CONTROL);
        if (isRunning)
        {
            creg &= ~_BV(DS3234_EOSC);
        }
        else
        {
            creg |= _BV(DS3234_EOSC);
        }
        setReg(DS3234_REG_CONTROL, creg);
    }

    void SetDateTime(const RtcDateTime& dt)
    {
        // clear the invalid flag
        uint8_t status = getReg(DS3234_REG_STATUS);
        status &= ~_BV(DS3234_OSF); // clear the flag
        setReg(DS3234_REG_STATUS, status);

        // set the date time
        _spi.beginTransaction(c_Ds3234SpiSettings);
        SelectChip();
        _spi.transfer(DS3234_REG_TIMEDATE | DS3234_REG_WRITE_FLAG);

        _spi.transfer(Uint8ToBcd(dt.Second()));
        _spi.transfer(Uint8ToBcd(dt.Minute()));
        _spi.transfer(Uint8ToBcd(dt.Hour())); // 24 hour mode only

        uint8_t year = dt.Year() - 2000;
        uint8_t centuryFlag = 0;

        if (year >= 100)
        {
            year -= 100;
            centuryFlag = _BV(7);
        }

        // RTC Hardware Day of Week is 1-7, 1 = Monday
        // convert our Day of Week to Rtc Day of Week
        uint8_t rtcDow = RtcDateTime::ConvertDowToRtc(dt.DayOfWeek());

        _spi.transfer(Uint8ToBcd(rtcDow));

        _spi.transfer(Uint8ToBcd(dt.Day()));
        _spi.transfer(Uint8ToBcd(dt.Month()) | centuryFlag);
        _spi.transfer(Uint8ToBcd(year));
        UnselectChip();
        _spi.endTransaction();
    }

    RtcDateTime GetDateTime()
    {
        _spi.beginTransaction(c_Ds3234SpiSettings);
        SelectChip();

        _spi.transfer(DS3234_REG_TIMEDATE);

        uint8_t second = BcdToUint8(_spi.transfer(0));
        uint8_t minute = BcdToUint8(_spi.transfer(0));
        uint8_t hour = BcdToBin24Hour(_spi.transfer(0));

        _spi.transfer(0); // throwing away day of week as we calculate it

        uint8_t dayOfMonth = BcdToUint8(_spi.transfer(0));
        uint8_t monthRaw = _spi.transfer(0);
        uint16_t year = BcdToUint8(_spi.transfer(0)) + 2000;

        UnselectChip();
        _spi.endTransaction();

        if (monthRaw & _BV(7)) // century wrap flag
        {
            year += 100;
        }
        uint8_t month = BcdToUint8(monthRaw & 0x7f);

        return RtcDateTime(year, month, dayOfMonth, hour, minute, second);
    }

    RtcTemperature GetTemperature()
    {
        _spi.beginTransaction(c_Ds3234SpiSettings);
        SelectChip();
        _spi.transfer(DS3234_REG_TEMP);

        // Temperature is represented as a 10-bit code with a resolution
        // of 1/4th �C and is accessable as a signed 16-bit integer at
        // locations 11h and 12h.
        //
        //       |         r11h          | DP |         r12h         |
        // Bit:   15 14 13 12 11 10  9  8   .  7  6  5  4  3  2  1  0  -1 -2
        //         s  i  i  i  i  i  i  i   .  f  f  0  0  0  0  0  0
        //
        // As it takes (8) right-shifts to register the decimal point (DP) to
        // the right of the 0th bit, the overall word scaling equals 256.
        //
        // For example, at +/- 25.25�C, concatenated registers <r11h:r12h> =
        // 256 * (+/- 25+(1/4)) = +/- 6464, or 1940h / E6C0h.

        int8_t  ms = _spi.transfer(0);  // MS byte, signed temperature
        uint8_t ls = _spi.transfer(0);  // LS byte is r12h

        UnselectChip();
        _spi.endTransaction();

        return RtcTemperature(ms, ls);  // LS byte is r12h
    }

    void Enable32kHzPin(bool enable)
    {
        uint8_t sreg = getReg(DS3234_REG_STATUS);

        if (enable == true)
        {
            sreg |= _BV(DS3234_EN32KHZ);
        }
        else
        {
            sreg &= ~_BV(DS3234_EN32KHZ);
        }

        setReg(DS3234_REG_STATUS, sreg);
    }

    void SetSquareWavePin(DS3234SquareWavePinMode pinMode)
    {
        uint8_t creg = getReg(DS3234_REG_CONTROL);

        // clear all relevant bits to a known "off" state
        creg &= ~(DS3234_AIEMASK | _BV(DS3234_BBSQW));
        creg |= _BV(DS3234_INTCN);  // set INTCN to disables SQW

        switch (pinMode)
        {
        case DS3234SquareWavePin_ModeNone:
            break;

        case DS3234SquareWavePin_ModeBatteryBackup:
            creg |= _BV(DS3234_BBSQW); // set battery backup flag
            creg &= ~_BV(DS3234_INTCN); // clear INTCN to enable SQW 
            break;

        case DS3234SquareWavePin_ModeClock:
            creg &= ~_BV(DS3234_INTCN); // clear INTCN to enable SQW 
            break;

        case DS3234SquareWavePin_ModeAlarmOne:
            creg |= _BV(DS3234_A1IE);
            break;

        case DS3234SquareWavePin_ModeAlarmTwo:
            creg |= _BV(DS3234_A2IE);
            break;

        case DS3234SquareWavePin_ModeAlarmBoth:
            creg |= _BV(DS3
```

## RtcDateTime.h

```cpp
/*-------------------------------------------------------------------------
RTC library

Written by Michael C. Miller.

I invest time and resources providing this open source code,
please support me by dontating (see https://github.com/Makuna/Rtc)

-------------------------------------------------------------------------
This file is part of the Makuna/Rtc library.

Rtc is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

Rtc is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with Rtc.  If not, see
<http://www.gnu.org/licenses/>.
-------------------------------------------------------------------------*/

#pragma once

// ESP32 complains if not included
#if defined(ARDUINO_ARCH_ESP32)
#include <inttypes.h>
#endif

#include "RtcTimeZone.h"
#include "RtcLocaleEnUs.h"
#include "RtcLocaleEn.h"

enum DayOfWeek
{
    DayOfWeek_Sunday = 0,
    DayOfWeek_Monday,
    DayOfWeek_Tuesday,
    DayOfWeek_Wednesday,
    DayOfWeek_Thursday,
    DayOfWeek_Friday,
    DayOfWeek_Saturday,
};

const uint16_t c_OriginYear = 2000;
const uint32_t c_UnixEpoch32 = 946684800; // Unix origin year is 1970
const uint32_t c_NtpEpoch32FromUnixEpoch32 = 2208988800; // Ntp origin year is 1900
const uint32_t c_NtpEpoch32 = c_UnixEpoch32 + c_NtpEpoch32FromUnixEpoch32;

extern const uint8_t c_daysInMonth[] PROGMEM;

const uint32_t c_MinuteAsSeconds = 60;
const uint32_t c_HourAsSeconds = 60 * c_MinuteAsSeconds;
const uint32_t c_DayAsSeconds = 24 * c_HourAsSeconds;
const uint32_t c_WeekAsSeconds = 7 * c_DayAsSeconds;


// AM PM
enum RtcMeridiem
{
    Rtc_AM,
    Rtc_PM
};

// handy conversion class between 24 hour and 12 hour values
// 
class RtcHourAmPm
{
public:
    RtcHourAmPm() :
        _hour(12),
        _meridiem(Rtc_AM)
    {
    }

    // construct from a 24 hour units with validation
    // if outside bounds, wrap into bounds
    //
    RtcHourAmPm(uint8_t hour24)
    {
        if (hour24 < 1 || hour24 > 23)
        {
            // midnight is 12am
            _hour = 12;
            _meridiem = Rtc_AM;
        }
        else if (hour24 < 12)
        {
            _hour = hour24;
            _meridiem = Rtc_AM;
        }
        else if (hour24 > 12)
        {
            _hour = hour24 - 12;
            _meridiem = Rtc_PM;
        }
        else
        {
            // noon is 12pm
            _hour = 12;
            _meridiem = Rtc_PM;
        }
    }

    // construct from 12 hour units and meridiem with validation
    //
    RtcHourAmPm(uint8_t hour12, RtcMeridiem meridiem)
    {
        // if outside bounds, wrap into bounds
        if (hour12 < 1)
        {
            hour12 = 12;
        }
        else if (hour12 > 12)
        {
            hour12 = 1;
            meridiem = (meridiem == Rtc_AM) ? Rtc_PM : Rtc_AM;
        }

        _hour = hour12;
        _meridiem = meridiem;
    }

    // covert to 24 hour units
    //
    operator uint8_t() const
    {
        uint8_t result = _hour;

        if (result == 12)
        {
            if (_meridiem == Rtc_AM)
            {
                result = 0;
            }
        }
        else if (_meridiem == Rtc_PM)
        {
            result += 12;
        }

        return result;
    }

    // properties
    //
    uint8_t Hour() const
    {
        return _hour;
    }

    RtcMeridiem Meridiem() const
    {
        return _meridiem;
    }

    uint8_t Hour24() const
    {
        return *this;
    }

protected:
    uint8_t _hour;
    RtcMeridiem _meridiem;
};


class RtcDateTime
{
public:
    explicit RtcDateTime(uint32_t secondsFrom2000 = 0) 
    {
        _initWithSecondsFrom2000<uint32_t>(secondsFrom2000);
    }

    RtcDateTime(uint16_t year,
        uint8_t month,
        uint8_t dayOfMonth,
        uint8_t hour,
        uint8_t minute,
        uint8_t second) :
        _yearFrom2000((year >= c_OriginYear) ? year - c_OriginYear : year),
        _month(month),
        _dayOfMonth(dayOfMonth),
        _hour(hour),
        _minute(minute),
        _second(second)
    {
    }

    // RtcDateTime compileDateTime(__DATE__, __TIME__);
    // sample input: date = "Dec 06 2009", time = "12:34:56"
    RtcDateTime(const char* date, const char* time) :
        RtcDateTime(0)
    {
        // __DATE__ is always in english
        InitWithDateTimeFormatString<RtcLocaleEnUs>(F("MMM DD YYYY"), date);
        InitWithDateTimeFormatString<RtcLocaleEnUs>(F("hh:mm:ss"), time);
    }

    bool IsValid() const;

    uint16_t Year() const
    {
        return c_OriginYear + _yearFrom2000;
    }

    uint8_t Month() const
    {
        return _month;
    }

    uint8_t Day() const
    {
        return _dayOfMonth;
    }

    uint8_t Hour() const
    {
        return _hour;
    }

    RtcHourAmPm HourAmPm() const
    {
        return RtcHourAmPm(_hour);
    }

    uint8_t Minute() const
    {
        return _minute;
    }

    uint8_t Second() const
    {
        return _second;
    }

    // 0 = Sunday, 1 = Monday, ... 6 = Saturday
    uint8_t DayOfWeek() const;

    // 32-bit time; as seconds since 1/1/2000
	uint32_t TotalSeconds() const;

	// 64-bit time; as seconds since 1/1/2000
	uint64_t TotalSeconds64() const;

	// total days since 1/1/2000
	uint16_t TotalDays() const;
	
    // return the next day that falls on the given day of week
    // if this day is that day of week, it will return this day
    RtcDateTime NextDayOfWeek(uint8_t dayOfWeek) const;

    /* 
    causes ambiguous overload for 'operator+'
    making explicit doesn't solve it
    // add unsigned seconds
    void operator += (uint32_t seconds)
    {
        *this = *this + seconds;
    }
    
    RtcDateTime operator + (uint32_t seconds) const
    {
        return RtcDateTime(TotalSeconds() + seconds);
    }
    */

    // add signed seconds
    void operator += (int32_t seconds)
    {
        *this = *this + seconds;
    }

    RtcDateTime operator + (int32_t seconds) const
    {
        uint32_t totalSeconds = TotalSeconds();
        // never allowed to go before year 2000
        if (seconds < 0 && static_cast<uint32_t>(abs(seconds)) > totalSeconds)
        {
            totalSeconds = 0;
        }
        else
        {
            totalSeconds += seconds;
        }
        return RtcDateTime(totalSeconds);
    }

    // remove seconds
    void operator -= (uint32_t seconds)
    {
        *this = *this - seconds;
    }

    RtcDateTime operator - (uint32_t seconds) const
    {
        return RtcDateTime(TotalSeconds() - seconds);
    }

    bool operator == (const RtcDateTime& right)
    {
        return (this->TotalSeconds() == right.TotalSeconds());
    }

    bool operator != (const RtcDateTime& right)
    {
        return (this->TotalSeconds() != right.TotalSeconds());
    }

    bool operator <= (const RtcDateTime& right)
    {
        return (this->TotalSeconds() <= right.TotalSeconds());
    }

    bool operator >= (const RtcDateTime& right)
    {
        return (this->TotalSeconds() >= right.TotalSeconds());
    }

    bool operator < (const RtcDateTime& right)
    {
        return (this->TotalSeconds() < right.TotalSeconds());
    }

    bool operator > (const RtcDateTime& right)
    {
        return (this->TotalSeconds() > right.TotalSeconds());
    }

    // Epoch32 support
    [[deprecated("Use Unix32Time() instead.")]]
    uint32_t Epoch32Time() const
    {
        return TotalSeconds() + c_UnixEpoch32;
    }
    [[deprecated("Use InitWithUnix32Time() instead.")]]
    void InitWithEpoch32Time(uint32_t secondsSince1970)
    {
        _initWithSecondsFrom2000<uint32_t>(secondsSince1970 - c_UnixEpoch32);
    }

    // Epoch64 support
    [[deprecated("Use Unix64Time() instead.")]]
    uint64_t Epoch64Time() const
    {
        return TotalSeconds64() + c_UnixEpoch32;
    }
    [[deprecated("Use InitWithUnix64Time() instead.")]]
    void InitWithEpoch64Time(uint64_t secondsSince1970)
    {
        _initWithSecondsFrom2000<uint64_t>(secondsSince1970 - c_UnixEpoch32);
    }

    // Unix32 support
    uint32_t Unix32Time() const
    {
        return TotalSeconds() + c_UnixEpoch32;
    }
    void InitWithUnix32Time(uint32_t secondsSince1970)
    {
        _initWithSecondsFrom2000<uint32_t>(secondsSince1970 - c_UnixEpoch32);
    }
    // Unix64 support
    uint64_t Unix64Time() const
    {
        return TotalSeconds64() + c_UnixEpoch32;
    }
    void InitWithUnix64Time(uint64_t secondsSince1970)
    {
        _initWithSecondsFrom2000<uint64_t>(secondsSince1970 - c_UnixEpoch32);
    }

    // Ntp32 support
    uint32_t Ntp32Time() const
    {
        return TotalSeconds() + c_NtpEpoch32;
    }
    void InitWithNtp32Time(uint32_t secondsSince1900)
    {
        _initWithSecondsFrom2000<uint32_t>(secondsSince1900 - c_NtpEpoch32);
    }
    // Ntp64 support
    uint64_t Ntp64Time() const
    {
        return TotalSeconds64() + c_NtpEpoch32;
    }
    void InitWithNtp64Time(uint64_t secondsSince1900)
    {
        _initWithSecondsFrom2000<uint64_t>(secondsSince1900 - c_NtpEpoch32);
    }

    [[deprecated("Use InitWithDateTimeFormatString()")]]
    void InitWithIso8601(const char* date)
    {
        // sample input: date = "Sat, 06 Dec 2009 12:34:56 GMT"
        InitWithDateTimeFormatString<RtcLocaleEnUs>(F("*, DD MMM YYYY hh:mm:ss zzz"), date);
    }

    //
    // https://www.w3.org/TR/NOTE-datetime
    // https://learn.microsoft.com/en-us/dotnet/standard/base-types/standard-date-and-time-format-strings
    // https://en.wikipedia.org/wiki/List_of_time_zone_abbreviations#:~:text=List%20of%20time%20zone%20abbreviations%20%20%20,%20%20UTC%2B08%3A00%20%2051%20more%20rows%20
    // 
    // * - ignore until next char
    // ! = ignore until not next char
    // 
    // YY - two digit year, assumes 2000 +
    // YYYY - four digit year
    // 
    // M - full month name, arbitrary length until next char
    // MM - two digit month
    // MMM - abreviated month name, 3 chars
    // 
    // DD - two digit day of month
    // 
    // hh - hour
    // mm - minute
    // ss - seconds
    // sssss - seconds with decimal (12.34)
    // 
    // z - +hh:mm or Z - 
    //      using this will adjust the time to UTC from the time zone
    //      using this will adjust the time to UTC from the time zone
    //      present in the string,
    //      without it, it will ignore the timezone and return the local
    // zzz - time zone abbreviation
    //      using this will adjust the time to UTC from the time zone
    //      present in the string,
    //      without it, it will ignore the timezone and return the local
    //
    // return - index last converted of datetime
    template <typename T_LOCALE = RtcLocaleEnUs> size_t InitWithDateTimeFormatString(
            const char* format, 
            const char* datetime)
    {
        const char specifiers[] = "*!YMDhmsz";
        const char* scan = format;
        const char* convert = datetime;
        int32_t timezoneMinutes = 0;

        // while chars in format and datetime
        while (*scan != '\0' && *datetime != '\0')
        {
            // find next token
            size_t iStart = strcspn(scan, specifiers);
            scan += iStart;
            convert += iStart;

            if (*scan != '\0')
            {
                // find the end of the token
                size_t iEnd = 1;
                while (scan[iEnd] == *scan)
                {
                    iEnd++;
                }
                size_t count = iEnd;
                size_t countConverted = 0;

                // handy debug tracing 
                //
                //Serial.print(scan[iStart]);
                //Se
```

## RtcLocaleEn.h

```cpp
/*-------------------------------------------------------------------------
RTC library

Written by Michael C. Miller.

I invest time and resources providing this open source code,
please support me by dontating (see https://github.com/Makuna/Rtc)

-------------------------------------------------------------------------
This file is part of the Makuna/Rtc library.

Rtc is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

Rtc is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with Rtc.  If not, see
<http://www.gnu.org/licenses/>.
-------------------------------------------------------------------------*/

#pragma once

class RtcLocaleEn
{
public:
    static uint8_t CharsToMonth(const char* monthChars, size_t count);

    // TimeZoneMinutesFromAbbreviation
    // searches known table for the abbreviation and provides the minutes offset
    //
    //   *minutes - [out] variable filled with the minutes offset 
    //   returns - the number of chars parsed, usually 3 or 4.  0 if not found 
    static size_t TimeZoneMinutesFromAbbreviation(int32_t* minutes, const char* abbr);
};
```

## RtcLocaleEnUs.h

```cpp
/*-------------------------------------------------------------------------
RTC library

Written by Michael C. Miller.

I invest time and resources providing this open source code,
please support me by dontating (see https://github.com/Makuna/Rtc)

-------------------------------------------------------------------------
This file is part of the Makuna/Rtc library.

Rtc is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

Rtc is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with Rtc.  If not, see
<http://www.gnu.org/licenses/>.
-------------------------------------------------------------------------*/

#pragma once

class RtcLocaleEnUs
{
public:
    static uint8_t CharsToMonth(const char* monthChars, size_t count);
    
    // TimeZoneMinutesFromAbbreviation
    // searches known table for the abbreviation and provides the minutes offset
    //
    //   *minutes - [out] variable filled with the minutes offset 
    //   returns - the number of chars parsed, usually 3 or 4.  0 if not found 
    static size_t TimeZoneMinutesFromAbbreviation(int32_t* minutes, const char* abbr);
};
```

## RtcPCF8563.h

```cpp
/*-------------------------------------------------------------------------
RTC library

Written by Michael C. Miller.

I invest time and resources providing this open source code,
please support me by dontating (see https://github.com/Makuna/Rtc)

-------------------------------------------------------------------------
This file is part of the Makuna/Rtc library.

Rtc is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

Rtc is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with Rtc.  If not, see
<http://www.gnu.org/licenses/>.
-------------------------------------------------------------------------*/

#pragma once

#include <Arduino.h>
#include "RtcUtility.h"
#include "RtcDateTime.h"


//I2C Slave Address
const uint8_t PCF8563_ADDRESS = 0x51;

//PCF8563 Register Addresses
const uint8_t PCF8563_REG_CONTROL = 0x00;
const uint8_t PCF8563_REG_STATUS = 0x01;
const uint8_t PCF8563_REG_TIMEDATE  = 0x02;
const uint8_t PCF8563_REG_INTEGRITY = 0x02;
const uint8_t PCF8563_REG_ALARM  = 0x09;
const uint8_t PCF8563_REG_CLKOUT_CONTROL = 0x0D;
const uint8_t PCF8563_REG_TIMER_CONTROL = 0x0E;
const uint8_t PCF8563_REG_TIMER  = 0x0F;

//PCF8563 Register Data Size if not just 1
const size_t PCF8563_REG_TIMEDATE_SIZE = 7;
const size_t PCF8563_REG_ALARM_SIZE = 4;

// PCF8563 Valid Register Bits (PCF8563_REG_INTEGRITY)
const uint8_t PCF8563_INTEGRITY_VALID = 7;

// PCF8563 Control Register Bits (PCF8563_REG_CONTROL)
const uint8_t PCF8563_CONTROL_CLOCK = 5;

// PCF8563 Status Register Bits (PCF8563_REG_STATUS)
const uint8_t PCF8563_STATUS_TIE = 0; // timer interrupt enable
const uint8_t PCF8563_STATUS_AIE = 1; // alarm interrupt enable
const uint8_t PCF8563_STATUS_TF = 2; // timer flag
const uint8_t PCF8563_STATUS_AF = 3; // alarm flag
const uint8_t PCF8563_STATUS_TITP = 4; // timer interrupt period config


enum PCF8563SquareWavePinMode
{
    PCF8563SquareWavePinMode_None  = 0b00000000,
    PCF8563SquareWavePinMode_32kHz = 0b10000000,
    PCF8563SquareWavePinMode_1kHz  = 0b10000001,
    PCF8563SquareWavePinMode_32Hz  = 0b10000010,
    PCF8563SquareWavePinMode_1Hz   = 0b10000011
};

// defines the timer period for the timer and
// when to trigger, minutes means at the top of the minute
// and period of 60 seconds
//
enum PCF8563TimerMode
{
    PCF8563TimerMode_None            = 0b00000000,
    PCF8563TimerMode_4096thOfASecond = 0b10000000,
    PCF8563TimerMode_64thOfASecond   = 0b10000001,
    PCF8563TimerMode_Seconds         = 0b10000010,
    PCF8563TimerMode_Minutes         = 0b10000011 // at the top of and by
};

enum PCF8563AlarmControlFlags
{
    PCF8563AlarmControl_MinuteMatch = 0x01,
    PCF8563AlarmControl_HourMatch = 0x02,
    PCF8563AlarmControl_DayOfMonthMatch = 0x04,
    PCF8563AlarmControl_DayOfWeekMatch = 0x08
};

class PCF8563Alarm
{
public:
    PCF8563Alarm(uint8_t dayOfMonth,
        uint8_t hour,
        uint8_t minute,
        uint8_t dayOfWeek,
        uint8_t controlFlags) :
        _flags(controlFlags),
        _dayOfMonth(dayOfMonth),
        _hour(hour),
        _minute(minute),
        _dayOfWeek(dayOfWeek)
    {
    }

    uint8_t DayOfMonth() const
    {
        return _dayOfMonth;
    }

    uint8_t Hour() const
    {
        return _hour;
    }

    uint8_t Minute() const
    {
        return _minute;
    }

    uint8_t DayOfWeek() const
    {
        return _dayOfWeek;
    }

    uint8_t ControlFlags() const
    {
        return _flags;
    }

    bool operator == (const PCF8563Alarm& other) const
    {
        return (_dayOfMonth == other._dayOfMonth &&
            _hour == other._hour &&
            _minute == other._minute &&
            _dayOfWeek == other._dayOfWeek &&
            _flags == other._flags);
    }

    bool operator != (const PCF8563Alarm& other) const
    {
        return !(*this == other);
    }

protected:
    uint8_t _flags;

    uint8_t _dayOfMonth;
    uint8_t _hour;
    uint8_t _minute;
    uint8_t _dayOfWeek;
};

template<class T_WIRE_METHOD> class RtcPCF8563
{
public:
    RtcPCF8563(T_WIRE_METHOD& wire) :
        _wire(wire),
        _lastError(Rtc_Wire_Error_None)
    {
    }

    void Begin()
    {
        _wire.begin();
    }

    void Begin(int sda, int scl)
    {
        _wire.begin(sda, scl);
    }

    uint8_t LastError()
    {
        return _lastError;
    }

    bool IsDateTimeValid()
    {
        uint8_t status = getReg(PCF8563_REG_INTEGRITY);
        return (!(status & _BV(PCF8563_INTEGRITY_VALID)) && (_lastError == Rtc_Wire_Error_None));
    }

    bool GetIsRunning()
    {
        uint8_t creg = getReg(PCF8563_REG_CONTROL);
        return (!(creg & _BV(PCF8563_CONTROL_CLOCK)) && (_lastError == Rtc_Wire_Error_None));
    }

    void SetIsRunning(bool isRunning)
    {
        uint8_t creg = getReg(PCF8563_REG_CONTROL);
        if (isRunning)
        {
            creg &= ~_BV(PCF8563_CONTROL_CLOCK);
        }
        else
        {
            creg |= _BV(PCF8563_CONTROL_CLOCK);
        }
        setReg(PCF8563_REG_CONTROL, creg);
    }

    void SetDateTime(const RtcDateTime& dt)
    {
        // the invalid flag is part of seconds,
        // so is automatically cleared when it is set

        // set the date time
        _wire.beginTransmission(PCF8563_ADDRESS);
        _wire.write(PCF8563_REG_TIMEDATE);

        _wire.write(Uint8ToBcd(dt.Second()));
        _wire.write(Uint8ToBcd(dt.Minute()));
        _wire.write(Uint8ToBcd(dt.Hour())); // 24 hour mode only
        _wire.write(Uint8ToBcd(dt.Day()));
        // RTC Hardware Day of Week is the same as RtcDateTime
        _wire.write(dt.DayOfWeek());

        uint8_t year = dt.Year() - 2000;
        uint8_t centuryFlag = 0;

        if (year >= 100)
        {
            year -= 100;
            centuryFlag = _BV(7);
        }

        _wire.write(Uint8ToBcd(dt.Month()) | centuryFlag);
        _wire.write(Uint8ToBcd(year));

        _lastError = _wire.endTransmission();
    }

    RtcDateTime GetDateTime()
    {
        _wire.beginTransmission(PCF8563_ADDRESS);
        _wire.write(PCF8563_REG_TIMEDATE);
        _lastError = _wire.endTransmission();
        if (_lastError != Rtc_Wire_Error_None)
        {
            return RtcDateTime(0);
        }

        size_t bytesRead = _wire.requestFrom(PCF8563_ADDRESS, PCF8563_REG_TIMEDATE_SIZE);
        if (PCF8563_REG_TIMEDATE_SIZE != bytesRead)
        {
            _lastError = Rtc_Wire_Error_Unspecific;
            return RtcDateTime(0);
        }

        uint8_t second = BcdToUint8(_wire.read() & 0x7F);
        uint8_t minute = BcdToUint8(_wire.read() & 0x7F);
        uint8_t hour = BcdToBin24Hour(_wire.read() & 0x3F);
        uint8_t dayOfMonth = BcdToUint8(_wire.read() & 0x3F);
        _wire.read();  // throwing away day of week as we calculate it
        uint8_t monthRaw = _wire.read();
        uint16_t year = BcdToUint8(_wire.read()) + 2000;

        if (monthRaw & _BV(7)) // century wrap flag
        {
            year += 100;
        }
        uint8_t month = BcdToUint8(monthRaw & 0x1F);


        return RtcDateTime(year, month, dayOfMonth, hour, minute, second);
    }

    void SetSquareWavePin(PCF8563SquareWavePinMode pinMode)
    {
        setReg(PCF8563_REG_CLKOUT_CONTROL, pinMode);
    }

    void SetAlarm(const PCF8563Alarm& alarm)
    {
        _wire.beginTransmission(PCF8563_ADDRESS);
        _wire.write(PCF8563_REG_ALARM);

        uint8_t matchFlag = (alarm.ControlFlags() & PCF8563AlarmControl_MinuteMatch) ? 0x00 : 0x80;
        _wire.write(Uint8ToBcd(alarm.Minute()) | matchFlag);

        matchFlag = (alarm.ControlFlags() & PCF8563AlarmControl_HourMatch) ? 0x00 : 0x80;
        _wire.write(Uint8ToBcd(alarm.Hour()) | matchFlag); // 24 hour mode only

        matchFlag = (alarm.ControlFlags() & PCF8563AlarmControl_DayOfMonthMatch) ? 0x00 : 0x80;
        _wire.write(Uint8ToBcd(alarm.DayOfMonth()) | matchFlag);

        matchFlag = (alarm.ControlFlags() & PCF8563AlarmControl_DayOfWeekMatch) ? 0x00 : 0x80;
        _wire.write(Uint8ToBcd(alarm.DayOfWeek()) | matchFlag);

        _lastError = _wire.endTransmission();

        if (_lastError == Rtc_Wire_Error_None)
        {
            // enable alarm
            uint8_t sreg = getReg(PCF8563_REG_STATUS);
            sreg &= ~_BV(PCF8563_STATUS_AF);
            sreg |= _BV(PCF8563_STATUS_AIE);
            setReg(PCF8563_REG_STATUS, sreg);
        }
    }

    void StopAlarm()
    {
        uint8_t sreg = getReg(PCF8563_REG_STATUS);
        sreg &= ~(_BV(PCF8563_STATUS_AIE) | _BV(PCF8563_STATUS_AF));
        setReg(PCF8563_REG_STATUS, sreg);
    }

    void SetTimer(PCF8563TimerMode mode, uint8_t time)
    {
        _wire.beginTransmission(PCF8563_ADDRESS);
        _wire.write(PCF8563_REG_TIMER_CONTROL);

        _wire.write(mode);
        _wire.write(time);

        _lastError = _wire.endTransmission();

        if (_lastError == Rtc_Wire_Error_None)
        {
            // enable timer
            uint8_t sreg = getReg(PCF8563_REG_STATUS);
            sreg &= ~_BV(PCF8563_STATUS_TF);
            sreg |= _BV(PCF8563_STATUS_TIE) | _BV(PCF8563_STATUS_TITP);
            setReg(PCF8563_REG_STATUS, sreg);
        }
    }

    void StopTimer()
    {
        uint8_t sreg = getReg(PCF8563_REG_STATUS);
        sreg &= ~_BV(PCF8563_STATUS_TF);
        sreg &= ~(_BV(PCF8563_STATUS_TIE) | _BV(PCF8563_STATUS_TITP));
        setReg(PCF8563_REG_STATUS, sreg);

        if (_lastError == Rtc_Wire_Error_None)
        {
            setReg(PCF8563_REG_TIMER_CONTROL, PCF8563TimerMode_None);
        }
    }

    // Latch must be called after an alarm otherwise it will not
    // trigger again?
    bool LatchAlarmTriggeredFlag()
    {
        uint8_t sreg = getReg(PCF8563_REG_STATUS);
        bool triggered = false;
        if (sreg & _BV(PCF8563_STATUS_AF))
        {
            triggered = true;
            sreg &= ~_BV(PCF8563_STATUS_AF);
            setReg(PCF8563_REG_STATUS, sreg);
        }
        return triggered;
    }

    bool LatchTimerTriggeredFlag()
    {
        uint8_t sreg = getReg(PCF8563_REG_STATUS);
        bool triggered = false;
        if (sreg & _BV(PCF8563_STATUS_TF))
        {
            triggered = true;
            sreg &= ~_BV(PCF8563_STATUS_TF);
            setReg(PCF8563_REG_STATUS, sreg);
        }
        return triggered;
    }

    bool GetAlarmTriggered()
    {
        uint8_t sreg = getReg(PCF8563_REG_STATUS);
        return (sreg & _BV(PCF8563_STATUS_AF));
    }

    bool GetTimerTriggered()
    {
        uint8_t sreg = getReg(PCF8563_REG_STATUS);
        return (sreg & _BV(PCF8563_STATUS_TF));
    }

private:
    T_WIRE_METHOD& _wire;
    uint8_t _lastError;

    uint8_t getReg(uint8_t regAddress)
    {
        _wire.beginTransmission(PCF8563_ADDRESS);
        _wire.write(regAddress);
        _lastError = _wire.endTransmission();
        if (_lastError != Rtc_Wire_Error_None)
        {
            return 0;
        }

        // control register
        size_t bytesRead = _wire.requestFrom(PCF8563_ADDRESS, (uint8_t)1);
        if (1 != bytesRead)
        {
            _lastError = Rtc_Wire_Error_Unspecific;
            return 0;
        }

        uint8_t regValue = _wire.read();
        return regValue;
    }

    void setReg(uint8_t regAddress, uint8_t regValue)
    {
        _wire.beginTransmission(PCF8563_ADDRESS);
        _wire.write(regAddress);
        _wire.write(regValue);
        _lastError = _wire.endTransmission();
    }

};
```

## RtcTemperature.h

```cpp
/*-------------------------------------------------------------------------
RTC library

Written by Michael C. Miller.

I invest time and resources providing this open source code,
please support me by dontating (see https://github.com/Makuna/Rtc)

-------------------------------------------------------------------------
This file is part of the Makuna/Rtc library.

Rtc is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

Rtc is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with Rtc.  If not, see
<http://www.gnu.org/licenses/>.
-------------------------------------------------------------------------*/

#pragma once


class RtcTemperature
{
public:
    // Constructor
	// a) Merge RTC registers into signed scaled temperature (x256),
	//    then bind to RTC resolution.
	//         |         r11h          | DP |         r12h         |
	// Bit:     15 14 13 12 11 10  9  8   .  7  6  5  4  3  2  1  0  -1 -2
	//           s  i  i  i  i  i  i  i   .  f  f  0  0  0  0  0  0
	//
	// b) Rescale to (x4) by right-shifting (6) bits
	//         |                                         | DP |    |
	// Bit:     15 14 13 12 11 10  9  8  7  6  5  4  3  2   .  1  0  -1 -2
	//           s  s  s  s  s  s  s  i  i  i  i  i  i  i      f  f   0  0
    RtcTemperature(int8_t highByteDegreesC, uint8_t lowByteDegreesC)
    {
        int16_t scaledDegC = ((highByteDegreesC << 8) | (lowByteDegreesC & 0xC0)) >> 6;
		_centiDegC = scaledDegC * 100 / 4;
    }

	RtcTemperature(int16_t centiDegC = 0) :
		_centiDegC(centiDegC)
	{
	}

    // Float temperature Celsius
    float AsFloatDegC()
    {
        return (float)_centiDegC / 100.0f;
    }

	// Float temperature Fahrenheit
	float AsFloatDegF()
	{
		return AsFloatDegC() * 1.8f + 32.0f;
	}

	// centi degrees (1/100th of a degree), 
	int16_t AsCentiDegC()
	{
		return _centiDegC;
	}

	void Print(Stream& target, uint8_t decimals = 2, char decimal = '.')
	{
		int16_t decimalDivisor = 1;
		int16_t integerPart;
		int16_t decimalPart;

		{
			int16_t rounded = abs(_centiDegC);
			// round up as needed
			if (decimals == 0)
			{
				rounded += 50;
			}
			else if (decimals == 1)
			{
				rounded += 5;
				decimalDivisor = 10;
			}

			integerPart = rounded / 100;
			decimalPart = (rounded % 100) / decimalDivisor;
		}

		// test for zero before printing negative sign to not print-0.00
		if (_centiDegC < 0 && (integerPart != 0 || decimalPart != 0))
		{
			target.print('-');
		}

		// print integer part
		target.print(integerPart);

		// print decimal part
		if (decimals != 0)
		{
			target.print(decimal);

			if (decimalPart != 0)
			{
				target.print(decimalPart);
			}
			else
			{
				// append zeros as requested
				while (decimals > 0)
				{
					target.print('0');
					decimals--;
				}
			}
		}
	}

	bool operator==(const RtcTemperature& other) const
	{
		return (_centiDegC == other._centiDegC);
	};

	bool operator>(const RtcTemperature& other) const
	{
		return (_centiDegC > other._centiDegC);
	};

	bool operator<(const RtcTemperature& other) const
	{
		return (_centiDegC < other._centiDegC);
	};

	bool operator>=(const RtcTemperature& other) const
	{
		return (_centiDegC >= other._centiDegC);
	};

	bool operator<=(const RtcTemperature& other) const
	{
		return (_centiDegC <= other._centiDegC);
	};

	bool operator!=(const RtcTemperature& other) const
	{
		return (_centiDegC != other._centiDegC);
	};

	RtcTemperature operator-(const RtcTemperature& right)
	{
		RtcTemperature result;

		result._centiDegC = (_centiDegC - right._centiDegC);
		return result;
	}

	RtcTemperature operator+(const RtcTemperature& right)
	{
		RtcTemperature result;

		result._centiDegC = (_centiDegC + right._centiDegC);
		return result;
	}

protected:
    int16_t  _centiDegC;  // 1/100th of a degree temperature (100 x degC)
};
```

## RtcTimeZone.h

```cpp
/*-------------------------------------------------------------------------
RTC library

Written by Michael C. Miller.

I invest time and resources providing this open source code,
please support me by dontating (see https://github.com/Makuna/Rtc)

-------------------------------------------------------------------------
This file is part of the Makuna/Rtc library.

Rtc is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

Rtc is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with Rtc.  If not, see
<http://www.gnu.org/licenses/>.
-------------------------------------------------------------------------*/

#pragma once

struct RtcTimeZone
{
    const char* abbr; // progmem
    int32_t offset;

	// searches the given table for the abbr match
	//  result - the found entry
	//  abbrSearch - the abbreviation to search for
	//  table - the PROGMEM table of RtcTimeZone in alphabetical order
	//  count - the number of entries in the table
	//  returns: 0 for not found, strlen of abbr if found
	//
    static size_t BinarySearchProgmemTable(RtcTimeZone* result,
        const char* abbrSearch,
        const RtcTimeZone* table,
        size_t countTable)
    {
		size_t indexFront = 0;
		size_t indexEnd = countTable;
		size_t index = countTable / 2;

		// binary search
		//
		while (indexFront < indexEnd)
		{
			RtcTimeZone entry;

			// copy the entry from progmem
			// string members still reside in PROGMEM though
			memcpy_P(&entry, table + index, sizeof(RtcTimeZone));

			auto match = strncmp_P(abbrSearch, entry.abbr, 4);

			if (0 == match)
			{
				// found
				*result = entry;
				return strlen_P(entry.abbr);
			}
			else if (match < 0)
			{
				indexEnd = index;
			}
			else
			{
				indexFront = index + 1;
			}

			// pick one between the front and end
			index = (indexEnd - indexFront) / 2 + indexFront;
		}

		return 0;
    }

	// searches the given table for the abbr match
	//  result - the found entry
	//  abbrSearch - the abbreviation to search for
	//  table - the PROGMEM table of RtcTimeZone to search
	//  count - the number of entries in the table
	//  returns: 0 for not found, strlen of abbr if found
	//
	static size_t LinearSearchProgmemTable(RtcTimeZone* result,
		const char* abbrSearch,
		const RtcTimeZone* table,
		size_t countTable)
	{
		for (size_t index = 0; index < countTable; index++)
		{
			RtcTimeZone entry;

			// copy the entry from progmem
			// string members still reside in PROGMEM though
			memcpy_P(&entry, table + index, sizeof(RtcTimeZone));

			if (0 == strncmp_P(abbrSearch, entry.abbr, 4))
			{
				*result = entry;
				return strlen_P(entry.abbr);
			}
		}

		return 0;
	}
};
```

## RtcUtility.h

```cpp
/*-------------------------------------------------------------------------
RTC library

Written by Michael C. Miller.

I invest time and resources providing this open source code,
please support me by dontating (see https://github.com/Makuna/Rtc)

-------------------------------------------------------------------------
This file is part of the Makuna/Rtc library.

Rtc is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

Rtc is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with Rtc.  If not, see
<http://www.gnu.org/licenses/>.
-------------------------------------------------------------------------*/

#pragma once

// ESP32 complains if not included
#if defined(ARDUINO_ARCH_ESP32)
#include <inttypes.h>
#endif

#if !defined(countof)
#define countof(a) (sizeof(a) / sizeof(a[0]))
#endif

// Arduino has no standard for attributing methods used for ISRs
// even though some platforms require it, so to simplify the problem
// for end users, this provides a standard ISR_ATTR
#if !defined(ISR_ATTR)

#if defined(ARDUINO_ARCH_ESP8266) 
#define ISR_ATTR ICACHE_RAM_ATTR
#elif defined(ARDUINO_ARCH_ESP32)
#define ISR_ATTR ICACHE_RAM_ATTR
#else
#define ISR_ATTR
#endif

#endif // !defined(ISR_ATTR)

// some platforms do not come with STL or properly defined one, specifically functional
// if you see...
// undefined reference to `std::__throw_bad_function_call()'
// ...then you can either add the platform symbol to the list so RTC_NO_STL gets defined or
// go to boards.txt and enable c++ by adding (teensy31.build.flags.libs=-lstdc++) and set to "smallest code" option in Arduino
//
#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_MEGAAVR) || defined(STM32L432xx) || defined(STM32L476xx) || defined(ARDUINO_ARCH_SAM)
#define RTC_NO_STL 1
#endif

// While WIRE has return codes, there is no standard definition of what they are
// within any headers; they are only documented on the website here
// https://www.arduino.cc/reference/en/language/functions/communication/wire/endtransmission/
// So we define our own "standard" for this RTC library that match those
//
enum Rtc_Wire_Error
{
	Rtc_Wire_Error_None = 0,
	Rtc_Wire_Error_TxBufferOverflow,
	Rtc_Wire_Error_NoAddressableDevice,
	Rtc_Wire_Error_UnsupportedRequest,
	Rtc_Wire_Error_Unspecific,
	Rtc_Wire_Error_CommunicationTimeout
};

// for some reason, the DUE board support does not define this, even though other non AVR archs do
#ifndef _BV
#define _BV(b) (1UL << (b))
#endif

extern uint8_t BcdToUint8(uint8_t val);
extern uint8_t Uint8ToBcd(uint8_t val);
extern uint8_t BcdToBin24Hour(uint8_t bcdHour);
```

## ThreeWire.h

```cpp
/*-------------------------------------------------------------------------
RTC library

Written by Michael C. Miller.

I invest time and resources providing this open source code,
please support me by dontating (see https://github.com/Makuna/Rtc)

-------------------------------------------------------------------------
This file is part of the Makuna/Rtc library.

Rtc is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

Rtc is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with Rtc.  If not, see
<http://www.gnu.org/licenses/>.
-------------------------------------------------------------------------*/

#include <Arduino.h>

#pragma once

//ThreeWire command Read/Write flag 
const uint8_t THREEWIRE_READFLAG = 0x01;

class ThreeWire 
{
public:
    ThreeWire(uint8_t ioPin, uint8_t clkPin, uint8_t cePin) :
        _ioPin(ioPin),
        _clkPin(clkPin),
        _cePin(cePin)
    {
    }

    void begin() {
        resetPins();
    }

    void end() {
        resetPins();
    }

    void beginTransmission(uint8_t command) {
        digitalWrite(_cePin, LOW); // default, not enabled
        pinMode(_cePin, OUTPUT);

        digitalWrite(_clkPin, LOW); // default, clock low
        pinMode(_clkPin, OUTPUT);

        pinMode(_ioPin, OUTPUT);

        digitalWrite(_cePin, HIGH); // start the session
        delayMicroseconds(4);           // tCC = 4us

        write(command, (command & THREEWIRE_READFLAG) == THREEWIRE_READFLAG);
    }

    void endTransmission() {
        digitalWrite(_cePin, LOW);
        delayMicroseconds(4);           // tCWH = 4us
    }

    void write(uint8_t value, bool isDataRequestCommand = false) {
        for (uint8_t bit = 0; bit < 8; bit++) {
            digitalWrite(_ioPin, value & 0x01);
            delayMicroseconds(1);     // tDC = 200ns

            // clock up, data is read by DS1302
            digitalWrite(_clkPin, HIGH);
            delayMicroseconds(1);     // tCH = 1000ns, tCDH = 800ns

            // for the last bit before a read
            // Set IO line for input before the clock down
            if (bit == 7 && isDataRequestCommand) {
                pinMode(_ioPin, INPUT);
            }

            digitalWrite(_clkPin, LOW);
            delayMicroseconds(1);     // tCL=1000ns, tCDD=800ns

            value >>= 1;
        }
    }

    uint8_t read() {
        uint8_t value = 0;

        for (uint8_t bit = 0; bit < 8; bit++) {
            // first bit is present on io pin, so only clock the other
            // bits
            value |= (digitalRead(_ioPin) << bit);
        
            // Clock up, prepare for next
            digitalWrite(_clkPin, HIGH);
            delayMicroseconds(1);

            // Clock down, value is ready after some time.
            digitalWrite(_clkPin, LOW);
            delayMicroseconds(1);        // tCL=1000ns, tCDD=800ns
        }

        return value;
    }

private:
    const uint8_t _ioPin;
    const uint8_t _clkPin;
    const uint8_t _cePin;

    void resetPins() {
        // just making sure they are in a default low power use state
        // as required state is set when transmissions are started
        // three wire devices have internal pull downs so they will be low
        pinMode(_clkPin, INPUT);
        pinMode(_ioPin, INPUT);
        pinMode(_cePin, INPUT);
    }
};
```

