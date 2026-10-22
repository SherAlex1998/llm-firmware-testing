#ifndef RTSL_h
#define RTSL_h

#define countof(a) (sizeof(a) / sizeof(a[0]))
#include <Wire.h>
#include <RtcDS1307.h>

#define MEMORY_ADDRESS 13

// Statistics storage in the DS1307 RAM: byte 0 = data address marker, byte 1 = data size, data at MEMORY_ADDRESS.
class RTSL {
public:
  RTSL(RtcDS1307<TwoWire>* rtc);
  void Init();
  void GetData(unsigned int* data);
  void SaveData(unsigned int* data);
  void HardResetData();
  RtcDateTime GetTime();

private:
  RtcDS1307<TwoWire>* rtc;
};

#endif
