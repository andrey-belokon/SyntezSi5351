#ifndef VFO_SI5351_H
#define VFO_SI5351_H

#if ARDUINO < 100
#include <WProgram.h>
#else
#include <Arduino.h>
#endif

#include "TRX.h"
#include <si5351.h>

class VFO_Si5351 : public VFO {
  private:
    long freq0, freq1, freq2;
    Si5351 si5351;
  public:
    void setup(long xtal_freq, long correction = 0,
      enum si5351_drive CLK0_Drive = SI5351_DRIVE_2MA,
      enum si5351_drive CLK1_Drive = SI5351_DRIVE_2MA,
      enum si5351_drive CLK2_Drive = SI5351_DRIVE_2MA
    );
    // устанавливает частоту в MHz на выводах
    // нулевая частота отключает выход
    void SetFreq(long f0, long f1, long f2);
    // устанавливает коэффициент коррекции частоты
    void SetCorrection(long correction);
    // выводит частоту калибрации на CLK_A. частота freq в MHz
    void OutCalibrationFreq(long freq);
};

#endif

