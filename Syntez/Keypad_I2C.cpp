#include "Keypad_I2C.h"
#include "i2c.h"

void KeypadI2C::setup() {
  i2c_init();
  pcf8574_write(0xFF);
}

int KeypadI2C::read_scan() {
  pcf8574_write(0xFF);
  for (int row=0; row <= 2; row++) {
    pcf8574_write(~(1<<row));
    switch (~(pcf8574_byte_read() >> 4) & 0xF) {
      case 0x1: return row;
      case 0x2: return 0x10+row;
      case 0x4: return 0x20+row;
      case 0x8: return 0x30+row;
    }
  }
  return -1;
}

int KeypadI2C::Read() {
  if (millis()-last_code_tm < 50) return -1;
  int code = read_scan(); 
  if (code == last_code) return -1;
  last_code = code;
  last_code_tm = millis();
  return code;
}

void KeypadI2C::pcf8574_write(int data) {
  i2c_begin_write(i2c_addr);
  i2c_write(data);
  i2c_end();
}

int KeypadI2C::pcf8574_byte_read() {
  i2c_begin_read(i2c_addr);
  int data = i2c_read();
  i2c_end();
  return data;
}
