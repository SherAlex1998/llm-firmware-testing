<!-- source: https://github.com/z3t0/Arduino-IRremote (version 2.8.0) | license: (c) Copyright 2009 Ken Shirriff http://www.righto.com -->

# IRremote library: API

Send and receive infrared signals with multiple protocols Currently included protocols: BoseWave, Denon, Dish, JVC, Lego, LG, MagiQuest, NEC, Panasonic, RC5, RC6, Samsung, Sanyo, Sharp, Sony, Whynter, (Pronto).<br/><br/><b>New: </b>Changed License to MIT, ATtiny timer 1 support, Removed AIWA + Mitsubishi, example IR2Keyboard.<br/>

## IRremote.h

```cpp
/**
 * @file IRremote.h
 * @brief Public API to the library.
 */

//******************************************************************************
// IRremote
// Version 2.0.1 June, 2015
// Initially coded 2009 Ken Shirriff http://www.righto.com
// Edited by Mitra to add new controller SANYO
//
// Interrupt code based on NECIRrcv by Joe Knapp
// http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1210243556
// Also influenced by http://zovirl.com/2008/11/12/building-a-universal-remote-with-an-arduino/
//
// JVC and Panasonic protocol added by Kristian Lauszus (Thanks to zenwheel and other people at the original blog post)
// LG added by Darryl Smith (based on the JVC protocol)
// Whynter A/C ARC-110WD added by Francesco Meschia
// MagiQuest added by E. Stuart Hicks (based on code by mpflaga - https://github.com/mpflaga/Arduino-IRremote/)
//******************************************************************************
#ifndef IRremote_h
#define IRremote_h

//------------------------------------------------------------------------------
#include "private/IRremoteInt.h"

/****************************************************
 *                     PROTOCOLS
 ****************************************************/
//------------------------------------------------------------------------------
// Supported IR protocols
// Each protocol you include costs memory and, during decode, costs time
// Disable (set to 0) all the protocols you do not need/want!
//
//#define DECODE_AIWA_RC_T501  1
//#define SEND_AIWA_RC_T501    1

#define DECODE_BOSEWAVE      1
#define SEND_BOSEWAVE        1

#define DECODE_DENON         1
#define SEND_DENON           1

#define DECODE_DISH          0 // NOT WRITTEN
#define SEND_DISH            1

#define DECODE_JVC           1
#define SEND_JVC             1

#define DECODE_LEGO_PF       1
#define SEND_LEGO_PF         1

#define DECODE_LG            1
#define SEND_LG              1

#define DECODE_MAGIQUEST     1
#define SEND_MAGIQUEST       1

//#define DECODE_MITSUBISHI    1 // Faulty implementation
//#define SEND_MITSUBISHI      0 // NOT WRITTEN

//#define USE_NEC_STANDARD // remove comment to have the standard NEC decoding (LSB first) available.
#if defined(USE_NEC_STANDARD)
#define DECODE_NEC_STANDARD  1
#define DECODE_NEC           0
#define LSB_FIRST_REQUIRED
#else
#define DECODE_NEC_STANDARD  0
#define DECODE_NEC           1
#endif
#define SEND_NEC             1
#define SEND_NEC_STANDARD    1

#define DECODE_PANASONIC     1
#define SEND_PANASONIC       1

#define DECODE_RC5           1
#define SEND_RC5             1

#define DECODE_RC6           1
#define SEND_RC6             1

#define DECODE_SAMSUNG       1
#define SEND_SAMSUNG         1

#define DECODE_SANYO         1
#define SEND_SANYO           0 // NOT WRITTEN

#define DECODE_SHARP         1
#define SEND_SHARP           1

#define DECODE_SHARP_ALT     1
#define SEND_SHARP_ALT       1
#if SEND_SHARP_ALT
#define LSB_FIRST_REQUIRED
#endif

#define DECODE_SONY          1
#define SEND_SONY            1

#define DECODE_WHYNTER       1
#define SEND_WHYNTER         1

#define DECODE_HASH          1 // special decoder for all protocols

/**
 * An enum consisting of all supported formats.
 * You do NOT need to remove entries from this list when disabling protocols!
 */
typedef enum {
    UNKNOWN = -1,
    UNUSED = 0,
//    AIWA_RC_T501,
    BOSEWAVE,
    DENON,
    DISH,
    JVC,
    LEGO_PF,
    LG,
    MAGIQUEST,
//    MITSUBISHI,
    NEC_STANDARD,
    NEC,
    PANASONIC,
    RC5,
    RC6,
    SAMSUNG,
    SANYO,
    SHARP,
    SHARP_ALT,
    SONY,
    WHYNTER,
} decode_type_t;

/**
 * Comment this out for lots of lovely debug output.
 */
//#define DEBUG
//------------------------------------------------------------------------------
// Debug directives
//
#ifdef DEBUG
#  define DBG_PRINT(...)    Serial.print(__VA_ARGS__)
#  define DBG_PRINTLN(...)  Serial.println(__VA_ARGS__)
#else
/**
 * If DEBUG, print the arguments, otherwise do nothing.
 */
#  define DBG_PRINT(...) void()
/**
 * If DEBUG, print the arguments as a line, otherwise do nothing.
 */
#  define DBG_PRINTLN(...) void()
#endif

//------------------------------------------------------------------------------
// Helper macro for getting a macro definition as string
//
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

//------------------------------------------------------------------------------
// Mark & Space matching functions
//
int MATCH(unsigned int measured, unsigned int desired);
int MATCH_MARK(uint16_t measured_ticks, unsigned int desired_us);
int MATCH_SPACE(uint16_t measured_ticks, unsigned int desired_us);

/****************************************************
 *                     RECEIVING
 ****************************************************/
/**
 * Results returned from the decoder
 */
struct decode_results {
    decode_type_t decode_type;  ///< UNKNOWN, NEC, SONY, RC5, ...
    uint16_t address;           ///< Used by Panasonic & Sharp6 NEC_standard [16-bits]
    uint32_t value;             ///< Decoded value / command [max 32-bits]
    uint16_t bits;              ///< Number of bits in decoded value
    uint16_t magnitude;         ///< Used by MagiQuest [16-bits]
    bool isRepeat;              ///< True if repeat of value is detected

    // next 3 values are copies of irparams values - see IRremoteint.h
    uint16_t *rawbuf;           ///< Raw intervals in 50uS ticks
    uint16_t rawlen;            ///< Number of records in rawbuf
    bool overflow;              ///< true if IR raw code too long
};

/**
 * DEPRECATED
 * Decoded value for NEC and others when a repeat code is received
 * Use Flag decode_results.isRepeat (see above) instead
 */
#define REPEAT 0xFFFFFFFF

/**
 * Main class for receiving IR
 */
class IRrecv {
public:
    /**
     * Instantiate the IRrecv class. Multiple instantiation is not supported.
     * @param recvpin Arduino pin to use. No sanity check is made.
     */
    IRrecv(int recvpin);
    /**
     * Instantiate the IRrecv class. Multiple instantiation is not supported.
     * @param recvpin Arduino pin to use, where a demodulating IR receiver is connected.
     * @param blinkpin pin to blink when receiving IR. Not supported by all hardware. No sanity check is made.
     */
    IRrecv(int recvpin, int blinkpin);

    /**
     * TODO: Why is this public???
     * @param blinkflag
     */
    void blink13(int blinkflag);

    /**
     * Attempt to decode the recently receive IR signal
     * @param results decode_results instance returning the decode, if any.
     * @return success of operation.
     */
    bool decode(decode_results *aResults);__attribute__ ((deprecated ("You should use decode() without a parameter.")))
    ;               // deprecated
    bool decode();

    /**
     * Enable IR reception.
     */
    void enableIRIn();

    /**
     * Disable IR reception.
     */
    void disableIRIn();

    /**
     * Returns status of reception
     * @return true if no reception is on-going.
     */
    bool isIdle();

    /**
     * Returns status of reception and copies IR-data to decode_results buffer if true.
     * @return true if data is available.
     */
    bool available();

    /**
     * Called to re-enable IR reception.
     */
    void resume();

    const char* getProtocolString();
    void printResultShort(Print *aSerial);
    void printIRResultRaw(Print *aSerial);
    void printIRResultRawFormatted(Print *aSerial);
    void printIRResultAsCArray(Print *aSerial);
    void printIRResultAsCVariables(Print *aSerial);
    /**
     * Print the result (second argument) as Pronto Hex on the Stream supplied as argument.
     * @param stream The Stream on which to write, often Serial
     * @param results the decode_results as delivered from irrecv.decode.
     * @param frequency Modulation frequency in Hz. Often 38000Hz.
     */
    void dumpPronto(Print *aSerial, unsigned int frequency = 38000U);
    void printIRResultAsPronto(Print *aSerial, unsigned int frequency = 38000U);

    bool decodePulseDistanceData(uint8_t aNumberOfBits, uint8_t aStartOffset, unsigned int aBitMarkMicros,
            unsigned int aOneSpaceMicros, unsigned int aZeroSpaceMicros, bool aMSBfirst = true);

    decode_results results; // the instance for decoding

private:
#if DECODE_HASH
    bool decodeHash();
    bool decodeHash(decode_results *aResults);
    int compare(unsigned int oldval, unsigned int newval);
#endif

    //......................................................................
#if DECODE_RC5
    /**
     * Try to decode the recently received IR signal as an RC5 signal-
     * @param results decode_results instance returning the decode, if any.
     * @return Success of the operation.
     */
    bool decodeRC5();
    bool decodeRC5(decode_results *aResults);
#endif
#if DECODE_RC6
    bool decodeRC6();
    bool decodeRC6(decode_results *aResults);
#endif
    //......................................................................
#if DECODE_NEC
    bool decodeNEC();
    bool decodeNEC(decode_results *aResults);
#endif
#if DECODE_NEC_STANDARD
    bool decodeNECStandard();
#endif

    //......................................................................
#if DECODE_SONY
    bool decodeSony();
    bool decodeSony(decode_results *aResults);
#endif
    //......................................................................
#if DECODE_PANASONIC
    bool decodePanasonic();
    bool decodePanasonic(decode_results *aResults);
#endif
    //......................................................................
#if DECODE_JVC
    bool decodeJVC();
    bool decodeJVC(decode_results *aResults);
#endif
    //......................................................................
#if DECODE_SAMSUNG
    bool decodeSAMSUNG();
    bool decodeSAMSUNG(decode_results *aResults);
#endif
    //......................................................................
#if DECODE_WHYNTER
    bool decodeWhynter();
    bool decodeWhynter(decode_results *aResults);
#endif
    //......................................................................
//#if DECODE_AIWA_RC_T501
//    bool decodeAiwaRCT501();
//    bool decodeAiwaRCT501(decode_results *aResults);
//#endif
    //......................................................................
#if DECODE_LG
    bool decodeLG();
    bool decodeLG(decode_results *aResults);
#endif
    //......................................................................
#if DECODE_SANYO
    bool decodeSanyo();
    bool decodeSanyo(decode_results *aResults);
#endif
    //......................................................................
//#if DECODE_MITSUBISHI
//    bool decodeMitsubishi();
//    bool decodeMitsubishi(decode_results *aResults);
//#endif
    //......................................................................
#if DECODE_DISH
      bool  decodeDish () ; // NOT WRITTEN
#endif
    //......................................................................
#if DECODE_SHARP
    bool decodeSharp();
    bool decodeSharp(decode_results *aResults);
#endif
#if DECODE_SHARP_ALT
    bool decodeSharpAlt();
    bool decodeSharpAlt(decode_results *aResults);
#endif
    //......................................................................
#if DECODE_DENON
    bool decodeDenon();
    bool decodeDenon(decode_results *aResults);
#endif
    //......................................................................
#if DECODE_LEGO_PF
      bool  decodeLegoPowerFunctions () ;
#endif
    //......................................................................
#if DECODE_BOSEWAVE
    bool decodeBoseWave();
    bool decodeBoseWave(decode_results *aResults);
#endif
    //......................................................................
#if DECODE_MAGIQUEST
    bool decodeMagiQuest();
    bool decodeMagiQuest(decode_results *aResults);
#endif
};

/****************************************************
 *                     SENDING
 ****************************************************/
```

## ir_Lego_PF_BitStreamEncoder.h

```cpp
//==============================================================================
//    L       EEEEEE   EEEE    OOOO
//    L       E       E       O    O
//    L       EEEE    E  EEE  O    O
//    L       E       E    E  O    O    LEGO Power Functions
//    LLLLLL  EEEEEE   EEEE    OOOO     Copyright (c) 2016, 2017 Philipp Henkel
//==============================================================================

//+=============================================================================
//

class LegoPfBitStreamEncoder {
private:
    uint16_t data;
    bool repeatMessage;
    uint8_t messageBitIdx;
    uint8_t repeatCount;
    uint16_t messageLength;

public:
    // HIGH data bit = IR mark + high pause
    // LOW data bit = IR mark + low pause
    static const uint16_t LOW_BIT_DURATION = 421;
    static const uint16_t HIGH_BIT_DURATION = 711;
    static const uint16_t START_BIT_DURATION = 1184;
    static const uint16_t STOP_BIT_DURATION = 1184;
    static const uint8_t IR_MARK_DURATION = 158;
    static const uint16_t HIGH_PAUSE_DURATION = HIGH_BIT_DURATION - IR_MARK_DURATION;
    static const uint16_t LOW_PAUSE_DURATION = LOW_BIT_DURATION - IR_MARK_DURATION;
    static const uint16_t START_PAUSE_DURATION = START_BIT_DURATION - IR_MARK_DURATION;
    static const uint16_t STOP_PAUSE_DURATION = STOP_BIT_DURATION - IR_MARK_DURATION;
    static const uint8_t MESSAGE_BITS = 18;
    static const uint16_t MAX_MESSAGE_LENGTH = 16000;

    void reset(uint16_t data, bool repeatMessage) {
        this->data = data;
        this->repeatMessage = repeatMessage;
        messageBitIdx = 0;
        repeatCount = 0;
        messageLength = getMessageLength();
    }

    int getChannelId() const {
        return 1 + ((data >> 12) & 0x3);
    }

    uint16_t getMessageLength() const {
        // Sum up all marks
        uint16_t length = MESSAGE_BITS * IR_MARK_DURATION;

        // Sum up all pauses
        length += START_PAUSE_DURATION;
        for (unsigned long mask = 1UL << 15; mask; mask >>= 1) {
            if (data & mask) {
                length += HIGH_PAUSE_DURATION;
            } else {
                length += LOW_PAUSE_DURATION;
            }
        }
        length += STOP_PAUSE_DURATION;
        return length;
    }

    boolean next() {
        messageBitIdx++;
        if (messageBitIdx >= MESSAGE_BITS) {
            repeatCount++;
            messageBitIdx = 0;
        }
        if (repeatCount >= 1 && !repeatMessage) {
            return false;
        } else if (repeatCount >= 5) {
            return false;
        } else {
            return true;
        }
    }

    uint8_t getMarkDuration() const {
        return IR_MARK_DURATION;
    }

    uint32_t getPauseDuration() const {
        if (messageBitIdx == 0)
            return START_PAUSE_DURATION;
        else if (messageBitIdx < MESSAGE_BITS - 1) {
            return getDataBitPause();
        } else {
            return getStopPause();
        }
    }

private:
    uint16_t getDataBitPause() const {
        const int pos = MESSAGE_BITS - 2 - messageBitIdx;
        const bool isHigh = data & (1 << pos);
        return isHigh ? HIGH_PAUSE_DURATION : LOW_PAUSE_DURATION;
    }

    uint32_t getStopPause() const {
        if (repeatMessage) {
            return getRepeatStopPause();
        } else {
            return STOP_PAUSE_DURATION;
        }
    }

    uint32_t getRepeatStopPause() const {
        if (repeatCount == 0 || repeatCount == 1) {
            return STOP_PAUSE_DURATION + (uint32_t) 5 * MAX_MESSAGE_LENGTH - messageLength;
        } else if (repeatCount == 2 || repeatCount == 3) {
            return STOP_PAUSE_DURATION + (uint32_t) (6 + 2 * getChannelId()) * MAX_MESSAGE_LENGTH - messageLength;
        } else {
            return STOP_PAUSE_DURATION;
        }
    }
};
```

