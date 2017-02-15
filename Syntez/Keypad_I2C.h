#ifndef KEYPADI2C_H
#define KEYPADI2C_H

#if ARDUINO < 100
#include <WProgram.h>
#else
#include <Arduino.h>
#endif

/*
 * матричная клавиатура 4х4 на PCF8574
 * строки в пинам 0..3, столбцы к пинам 4..7
 */

class KeypadI2C {
  private:
    byte i2c_addr;
    int last_code;
    long last_code_tm;
    
    void pcf8574_write(int data);
    int pcf8574_byte_read();
  public:
	  KeypadI2C(int _i2c_addr): i2c_addr(_i2c_addr), last_code(-1), last_code_tm(0) {}
    
    void setup();
    
    // "сырое" чтение без подавления дребезга
    int read_scan();
    // возвращает младшие 4 бита - строка, старшие - столбец
    // -1 если ничего не нажато. подавляет дребезг
    int Read();
    // последний прочитанный
    int GetLastCode() { return last_code; }
};

#endif
