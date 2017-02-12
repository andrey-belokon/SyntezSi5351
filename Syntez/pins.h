#ifndef PINS_H
#define PINS_H

#if ARDUINO < 100
#include <WProgram.h>
#else
#include <Arduino.h>
#endif

// pin with bouncing, pull up and active low level
class InputPullUpPin {
  private:
	  int pin;
	  bool last;
	  long last_tm;
  public:
    InputPullUpPin(int _pin):pin(_pin),last(false),last_tm(0) {}
    void setup();
    bool Read();
};

class InputAnalogPin {
  private:
	  int pin,min_val,max_val,value,rfac,pool_interval;
	  long last_pool_tm;
  public:
	  InputAnalogPin(int _pin, int def_value, int _min_val, int _max_val, int _rfac=0, int _pool_interval = 20):
	  pin(_pin),min_val(_min_val),max_val(_max_val),value(def_value),
	  rfac(_rfac),pool_interval(_pool_interval),last_pool_tm(0) {}
    void setup();
    int Read();
};

class OutputBinPin {
  private:
	  int pin,active_level,def_value,state;
  public:
	  OutputBinPin(int _pin, int _def_value, int _active_level):
	  pin(_pin),active_level(_active_level),def_value(_def_value),state(-1) {}
    void setup();
    void Write(bool value);
};

class OutputTonePin {
  private:
    int pin,freq;
  public:
    OutputTonePin(int _pin, int _freq):pin(_pin),freq(_freq) {}
    void setup();
    void Write(bool value);
};

class OutputPCF8574 {
  private:
	  int i2c_addr;
  	byte value,old_value;
    void pcf8574_write(int data);
  public:
	  OutputPCF8574(int _i2c_addr, byte init_state):i2c_addr(_i2c_addr),value(init_state),old_value(init_state) {}
    void setup();
	  void Set(int pin, bool state);
	  void Write();
};

#endif
