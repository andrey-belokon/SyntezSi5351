#ifndef DISP_1602_H
#define DISP_1602_H

#if ARDUINO < 100
#include <WProgram.h>
#else
#include <Arduino.h>
#endif

#include "TRX.h"
#include "LCD1602_I2C.h"

class Display_1602_I2C: public TRXDisplay {
  private:
  	LiquidCrystal_I2C lcd;
    bool tx;
  public:
	  Display_1602_I2C (int i2c_addr): lcd(i2c_addr,16,2) {}
	  void setup();
	  void Draw(TRX& trx);
    void clear() { lcd.clear(); }
    void DrawMenu(const char* title, const char** items, byte selected, const char* help, byte fontsize);
    void DrawCalibration(const char* title, long value, bool hi_res, const char* help = NULL);
};

#endif
