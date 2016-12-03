bool VFO_CalibrationMenu(
  Si5351& vfo,
  TRXDisplay& disp,
  KeypadI2C& keypad,
  Encoder& encoder,
  long xtal_freq,
  long& correction,
  long calibrate_freq = 30000000) {
  int keycode = keypad.read_scan();
  if (keycode >= 0) {
    TRXCommand cmd = KeyMap[keycode & 0xF][keycode >> 4];
    if (cmd == cmdBandUp) {
      String calibrate_title(F("CALIBRATE SI5351"));
      // крутим энкодер пока на выходе VFO1 не будет частота "по нулям"
      // потом нажимаем btBandDown
      // выход с отменой - btBandUp
      // brAttPre - смена шага на мелкий/крупный (по дефолту крупный)
      // btVFOSel - сброс в ноль
      long curr_correction = correction;
      long last_correction = 0;
      int freq_step = 32;
      disp.DrawCalibration(calibrate_title.c_str(),curr_correction,false);
      vfo.set_freq(calibrate_freq,0,0);
      vfo.set_xtal_freq(xtal_freq+curr_correction);
      while (keypad.read_scan() == keycode) {
        delay(100);
      }
      while (true) {
        curr_correction -= encoder.GetDelta()*freq_step/32;
        disp.DrawCalibration(calibrate_title.c_str(),curr_correction,freq_step == 1);
        if (curr_correction != last_correction) {
          vfo.set_xtal_freq(xtal_freq+curr_correction,0);
          last_correction = curr_correction;
        }
        keycode = keypad.Read();
        if (keycode >= 0) {
          cmd = KeyMap[keycode & 0xF][keycode >> 4];
          if (cmd == cmdAttPre) {
            freq_step = (freq_step == 1 ? 32 : 1);
          } else if (cmd == cmdVFOSel) {
            curr_correction = 0;
          } else if (cmd == cmdBandUp) {
            vfo.set_xtal_freq(xtal_freq+correction);
            return false;
          } else if (cmd == cmdBandDown) {
            String save_title(F("SAVE CALIBRATION"));
            disp.DrawCalibration(save_title.c_str(),curr_correction,false);
            correction = curr_correction;
            vfo.set_xtal_freq(xtal_freq+correction);
            return true;
          }
        }
      }
    }
  }
  return false;
}

