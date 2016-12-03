#include "i2c.h"
#include "pins.h"

bool InputPullUpPin::Read() {
  if (pin < 0) return false;
  if (millis()-last_tm < 50) return last;
  bool val = digitalRead(pin) == LOW;
  if (val != last) {
	  last = val;
	  last_tm = millis();
  }
  return val;
}

void InputPullUpPin::setup() {
  if (pin >= 0) 
    pinMode(pin, INPUT_PULLUP); 
}

void InputAnalogPin::setup() {
  if (pin >= 0) 
    pinMode(pin, INPUT); 
}

int InputAnalogPin::Read() {
  if (pin >= 0 && millis()-last_pool_tm >= pool_interval) {
    value = (max_val-min_val)*(long)analogRead(pin)/1024L + min_val;
  }
  return value;
}

void OutputTonePin::setup() {
  if (pin >= 0) 
    pinMode(pin, OUTPUT); 
}

void OutputTonePin::Write(bool value) { 
  if (pin >= 0) {
    if (value) tone(pin,freq); 
    else noTone(pin); 
  }
}

void OutputPin::setup() {
  if (pin >= 0) {
	  pinMode(pin, OUTPUT);
	  Write(def_value);
  }
}

void OutputPin::Write(bool value) {
  if (pin >= 0 && state != value) {
	  digitalWrite(pin,value?(active_level == HIGH?HIGH:LOW):(active_level == HIGH?LOW:HIGH));
	  state = value;
  }
}

void OutputPCF8574::setup() {
  pcf8574_write(value);
}

void OutputPCF8574::Set(int pin, bool state) {
  if (state) value |= (1 << (pin & 0x7));
  else value &= ~(1 << (pin & 0x7));
}

void OutputPCF8574::Write() {
  if (value != old_value) {
	  pcf8574_write(value);
	  old_value = value;
  }
}

void OutputPCF8574::pcf8574_write(int data) {
  if (i2c_addr >= 0) {
	  i2c_begin_write(i2c_addr);
	  i2c_write(data);
    i2c_end();
  }
}


