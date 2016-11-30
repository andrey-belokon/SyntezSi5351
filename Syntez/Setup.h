bool VFO_CalibrationMenu(
  VFO& vfo,
  TRXDisplay& disp,
  KeypadI2C& keypad,
  Encoder& encoder,
  long& vfo_calibration_value,
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
      long delta = vfo_calibration_value;
      int freq_step = 8;
      disp.DrawCalibration(calibrate_title.c_str(),0,false);
      vfo.SetCorrection(0);
      vfo.OutCalibrationFreq(calibrate_freq);
      while (keypad.read_scan() == keycode) {
        delay(100);
      }
      while (true) {
        delta -= encoder.GetDelta()*freq_step;
        disp.DrawCalibration(calibrate_title.c_str(),delta,freq_step == 1);
        vfo.SetCorrection(delta);
        keycode = keypad.Read();
        if (keycode >= 0) {
          cmd = KeyMap[keycode & 0xF][keycode >> 4];
          if (cmd == cmdAttPre) {
            freq_step = (freq_step == 8 ? 1 : 8);
          } else if (cmd == cmdVFOSel) {
            delta = 0;
          } else if (cmd == cmdBandUp) {
            vfo.SetCorrection(vfo_calibration_value);
            return false;
          } else if (cmd == cmdBandDown) {
            String save_title(F("SAVE CALIBRATION"));
            disp.DrawCalibration(save_title.c_str(),delta,false);
            vfo_calibration_value = delta;
            return true;
          }
        }
      }
    }
  }
  return false;
}

