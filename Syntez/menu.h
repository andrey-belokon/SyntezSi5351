///////////////////////////// menu functions ////////////////////////

#include "TinyRTC.h"

void PrintRTCData(RTCData *dt, char *buf, char **items)
{
  *items++ = buf;
  buf += sprintf(buf,"Day %x ",dt->day)+1;
  *items++ = buf;
  buf += sprintf(buf,"Month %x ",dt->month)+1;
  *items++ = buf;
  buf += sprintf(buf,"Year %x ",dt->year)+1;
  *items++ = buf;
  buf += sprintf(buf,"Hour %x ",dt->hour)+1;
  *items++ = buf;
  buf += sprintf(buf,"Minute %x ",dt->min)+1;
  *items++ = buf;
  buf += sprintf(buf,"Second %x ",dt->sec)+1;
  *items = NULL;
}

byte dec2bcd(byte val)
{
  return( (val/10*16) + (val%10));
}

byte bcd2dec(byte val)
{
  return( (val/16*10) + (val%16));
}

void ShowClockMenu()
{
  const char* title = "Clock setup";
  const char* help = "Lock - exit no save\nAttPre - save & exit\nrotate encoder for change";
  char buf[64];
  char *items[7];
  RTCData dt;
  byte selected=0;
  long encval;
  RTC_Read(&dt,0,sizeof(dt));
  dt.sec=0;
  disp.clear();
  PrintRTCData(&dt,buf,items);
  disp.DrawMenu(title,(const char**)items,selected,help,2);
  while (1) {
    int keycode=keypad.Read();
    if (keycode >= 0) {
      switch (KeyMap[keycode & 0xF][keycode >> 4]) {
        case cmdBandUp:
          if (selected > 0) selected--;
          else selected=5;
          disp.DrawMenu(title,(const char**)items,selected,help,2);
          encval=0;
          break;
        case cmdBandDown:
          if (selected < 5) selected++;
          else selected=0;
          disp.DrawMenu(title,(const char**)items,selected,help,2);
          encval=0;
          break;
        case cmdAttPre:
          RTC_Write(&dt);
          return;
        case cmdLock:
          return;
      }
    }
    encval += encoder.GetDelta();
    int delta = encval / 500;
    if (delta != 0) {
      switch (selected) {
        case 0:
          dt.day = dec2bcd(bcd2dec(dt.day)+delta);
          if (dt.day > 0x31) dt.day=0x31;
          break;
        case 1:
          dt.month = dec2bcd(bcd2dec(dt.month)+delta);
          if (dt.month > 0x12) dt.month=0x12;
          break;
        case 2:
          dt.year = dec2bcd(bcd2dec(dt.year)+delta);
          if (dt.year > 0x99) dt.year=0x99;
          break;
        case 3:
          dt.hour = dec2bcd(bcd2dec(dt.hour)+delta);
          if (dt.hour > 0x24) dt.hour=0x24;
          break;
        case 4:
          dt.min = dec2bcd(bcd2dec(dt.min)+delta);
          if (dt.min > 0x59) dt.min=0x59;
          break;
        case 5:
          dt.sec = dec2bcd(bcd2dec(dt.sec)+delta);
          if (dt.sec > 0x59) dt.sec=0x59;
          break;
      }
      PrintRTCData(&dt,buf,items);
      disp.DrawMenu(title,(const char**)items,selected,help,2);
      encval=0;
    }
  }
}

void PrintSMeterData(int *dt, char *buf, char **items)
{
  for (byte i=0; i < 15; i++) {
    *items++ = buf;
    if (i < 9)
      buf += sprintf(buf,"S%u %u   ",i+1,dt[i])+1;
    else
      buf += sprintf(buf,"+%u0 %u   ",i-8,dt[i])+1;
  }
  *items++ = buf;
  buf += sprintf(buf,"Save & Exit")+1;
  *items = NULL;
}

void ShowSMeterMenu()
{
  const char*help = "BandUp,BandDown - navigation\nAttPre - set value\nLock - exit no save";
  int smeter[15];
  char buf[200];
  char title[16];
  char *items[17];
  byte selected=0;
  memcpy(smeter,SMeterMap,sizeof(smeter));
  disp.clear();
  PrintSMeterData(smeter,buf,items);
  while (1) {
    sprintf(title,"AGC=%u   ",inSMeter.Read());
    disp.DrawMenu(title,(const char**)items,selected,help,1);
    int keycode=keypad.Read();
    if (keycode >= 0) {
      switch (KeyMap[keycode & 0xF][keycode >> 4]) {
        case cmdBandUp:
          if (selected > 0) selected--;
          else selected=15;
          disp.DrawMenu(title,(const char**)items,selected,help,1);
          break;
        case cmdBandDown:
          if (selected < 15) selected++;
          else selected=0;
          disp.DrawMenu(title,(const char**)items,selected,help,1);
          break;
        case cmdAttPre:
          if (selected < 15) {
            smeter[selected]=inSMeter.Read();
            PrintSMeterData(smeter,buf,items);
          } else {
            memcpy(SMeterMap,smeter,sizeof(SMeterMap));
            eeprom_write_block(SMeterMap, SMeterMap_EEMEM, sizeof(SMeterMap));
            return;
          }
          break;
        case cmdLock:
          return;
      }
    }
  }
}

void ShowSi5351CalibrationMenu()
{
  const char *calibrate_title="CALIBRATE SI5351";
  const char *help = "BandUp - change step\nBandDown - set to zero\nLock - exit no save\nAttPre - save & exit\nrotate encoder for change";
  // крутим энкодер пока на выходе VFO1 не будет частота "по нулям"
  // потом нажимаем btBandDown
  // выход с отменой - btBandUp
  // brAttPre - смена шага на мелкий/крупный (по дефолту крупный)
  // btVFOSel - сброс в ноль
  long curr_correction = Si5351_correction;
  long last_correction = 0;
  int freq_step = 32;
  disp.clear();
  disp.DrawCalibration(calibrate_title,curr_correction,false,help);
  vfo.set_freq(SI5351_CALIBRATION_FREQ,0,0);
  vfo.set_xtal_freq(SI5351_XTAL_FREQ+curr_correction);
  while (true) {
    curr_correction -= encoder.GetDelta()*freq_step/32;
    disp.DrawCalibration(calibrate_title,curr_correction,freq_step == 1,help);
    if (curr_correction != last_correction) {
      vfo.set_xtal_freq(SI5351_XTAL_FREQ+curr_correction,0);
      last_correction = curr_correction;
    }
    int keycode = keypad.Read();
    if (keycode >= 0) {
      switch (KeyMap[keycode & 0xF][keycode >> 4]) {
        case cmdBandUp:
          freq_step = (freq_step == 1 ? 32 : 1);
          break;
        case cmdBandDown:
          curr_correction = 0;
          break;
        case cmdLock:
          vfo.set_xtal_freq(SI5351_XTAL_FREQ+Si5351_correction);
          return;
        case cmdAttPre:
          disp.DrawCalibration("SAVE CALIBRATION",curr_correction,false,help);
          Si5351_correction = curr_correction;
          vfo.set_xtal_freq(SI5351_XTAL_FREQ+Si5351_correction);
          eeprom_write_block(&Si5351_correction, &Si5351_correction_EEMEM, sizeof(Si5351_correction));
          delay(2000);
          return;
      }
    }
  }
}

void ShowMenu()
{
  const char* title = "Main menu";
  const char* MenuItems[] = {"Clock","Si5351","S-Meter","Exit",NULL};
  const char* help = "BandUp/BandDown - move\nAttPre - select\nLock - exit";
  byte selected=0;
  disp.clear();
  disp.DrawMenu(title,MenuItems,selected,help,2);
  while (true) {
    int keycode=keypad.Read();
    if (keycode >= 0) {
      switch (KeyMap[keycode & 0xF][keycode >> 4]) {
        case cmdBandUp:
          if (selected > 0) selected--;
          else selected=3;
          disp.DrawMenu(title,MenuItems,selected,help,2);
          break;
        case cmdBandDown:
          if (selected < 3) selected++;
          else selected=0;
          disp.DrawMenu(title,MenuItems,selected,help,2);
          break;
        case cmdAttPre:
          switch (selected) {
            case 0:
              if (RTC_found()) ShowClockMenu();
              break;
            case 1:
              ShowSi5351CalibrationMenu();
              break;
            case 2:
              ShowSMeterMenu();
              break;
            case 3:
              return;
          }
          // redraw
          disp.clear();
          disp.DrawMenu(title,MenuItems,selected,help,2);
          break;
        case cmdLock:
          disp.clear();
          return;
      }
    }
  }
}


