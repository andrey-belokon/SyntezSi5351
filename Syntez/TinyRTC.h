#ifndef TINYRTC_H
#define TINYRTC_H

#if ARDUINO < 100
#include <WProgram.h>
#else
#include <Arduino.h>
#endif

// all data in BCD format (hex as decimal)
typedef struct {
  byte sec;     // 0-59
  byte min;     // 0-59
  byte hour;    // 1-23
  byte dow;     // day of week 1-7
  byte day;     // 1-28/29/30/31
  byte month;   // 1-12
  byte year;    // 0-99
} RTCData;

void RTC_Write(RTCData* data);

void RTC_Read(void *data, byte start, byte count);

bool RTC_found();

#endif
