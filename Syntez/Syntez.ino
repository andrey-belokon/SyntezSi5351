//
// Синтезатор UR5FFR
// Copyright (c) Andrey Belokon, 2016 Odessa 
// https://github.com/andrey-belokon/SyntezSi5351
//

// раскоментарить тип используемого дисплея (только один!)
//#define DISPLAY_LCD_1602
#define DISPLAY_TFT_ILI9341

#include <avr/eeprom.h> 
#include "Encoder.h"
#include "Keypad_I2C.h"
#include "TinyRTC.h"
#include "pins.h"
#include "TRX.h"
#ifdef DISPLAY_LCD_1602
  #include "disp_1602.h"
#endif
#ifdef DISPLAY_TFT_ILI9341
  #include "disp_ILI9341.h"
#endif
#include "si5351a.h"
#include "i2c.h"

#define RIT_MAX_VALUE   1200      // максимальная расстройка
#define MENU_DELAY      1000      // задержка вызова меню в сек

// мапинг сканкодов на команды
const TRXCommand KeyMap[4][4] = {
  cmdBandUp, cmdBandDown, cmdAttPre, cmdVFOSel,
  cmdVFOEQ,  cmdUSBLSB,   cmdLock,   cmdSplit,
  cmdRIT,    cmdHam,      cmdZero,   cmdQRP,
  cmdNone,   cmdNone,     cmdNone,   cmdNone
};

KeypadI2C keypad(0x26);
Encoder encoder(360);
#ifdef DISPLAY_LCD_1602
  Display_1602_I2C disp(0x27);
#endif
#ifdef DISPLAY_TFT_ILI9341
  Display_ILI9341_SPI disp;
#endif
TRX trx;

#define SI5351_XTAL_FREQ         270000000  // 0.1Hz resolution (10x mutiplier)
#define SI5351_CALIBRATION_FREQ  30000000   // частота на которой проводится калибровка
long EEMEM Si5351_correction_EEMEM = 0;
long Si5351_correction;
Si5351 vfo;

int EEMEM SMeterMap_EEMEM[15] = {70,140,210,280,350,420,490,560,630,700,770,840,910,940,980};
int SMeterMap[15];

InputPullUpPin inTX(4);
InputPullUpPin inTune(5);
InputAnalogPin inRIT(A0,5);
InputAnalogPin inSMeter(A1);
OutputBinPin outTX(6,false,HIGH);
OutputBinPin outQRP(7,false,HIGH);
OutputTonePin outTone(8,1000);

OutputPCF8574 outBandCtrl(0x25,0);
// распиновка I2C расширителя:
// двоичный деифратор диапазона - пины 0-3
const int pnBand0 = 0;
const int pnBand1 = 1;
const int pnBand2 = 2;
const int pnBand3 = 3;
// 5й пин - ATT, 6й пин - Preamp
const int pnAtt = 5;
const int pnPre = 6;

void setup()
{
  i2c_init();
  eeprom_read_block(&Si5351_correction, &Si5351_correction_EEMEM, sizeof(Si5351_correction));
  eeprom_read_block(SMeterMap, SMeterMap_EEMEM, sizeof(SMeterMap));
  //Serial.begin(9600);
  vfo.setup(1,0,0);
  vfo.set_xtal_freq(SI5351_XTAL_FREQ+Si5351_correction);
  encoder.setup();
  keypad.setup();
  inTX.setup();
  inTune.setup();
  inRIT.setup();
  outTX.setup();
  outQRP.setup();
  outTone.setup();
  disp.setup();
  trx.SwitchToBand(1);
}

// необходимо раскоментировать требуемую моду (только одну!)

// режим прямого преобразования. частота формируется на 1ом выводе. установить
// CLK0_MULT в значение 1/2/4 в зависимости от коэффициента деления частоты гетеродина
// второй и третий гетеродины отключены
//#define MODE_DC

// режим прямого преобразования с формированием квадратурн
// частота формируется на выводах CLK0,CLK1 со сдвигом фаз 90град
// CLK0_MULT не используется. CLK2 отключен. Минимальная частота настройки 2MHz (по даташиту 4MHz)
//#define MODE_DC_QUADRATURE

// одна промежуточная частота. требуемая боковая формируется на счет переключения
// первого гетеродина "сверху" (IF=VFO-Fc) или "снизу" (IF=Fc-VFO)
// подразумевается что используется USB КФ 
// второй и третий гетеродины отключены. 
// режим предназначен для трактов где второй гетеродин кварцованый на фиксированную чатсоту
//#define MODE_SINGLE_IF_VFOSB

// одна промежуточная частота. первый гетеродин на выходе CLK0 всегда "сверху" (IF=VFO-Fc)
// требуемая боковая формируется на счет переключения второго гетеродина на выходе CLK1
// третий гетеродин отключен
//#define MODE_SINGLE_IF

// режим аналогичен MODE_SINGLE_IF но в второй гетеродин генерируется на CLK1 при RX и
// на CLK2 в режиме TX
// для трактов котрые имеют отдельные смесители для формирования/детектирования сигнала
#define MODE_SINGLE_IF_RXTX

// режим аналогичен MODE_SINGLE_IF но в режиме передачи гетеродины комутируются,
// тоесть первый формируется на CLK1, а второй - на CLK0
// для трактов где необходимо переключение гетеродинов при смене RX/TX
// третий гетеродин отключен
//#define MODE_SINGLE_IF_SWAP

// две промежуточные частоты. гетеродины формируются 1й - CLK0, 2й - CLK1, 3й - CLK2
// первый гетеродин всегда "сверху". выбор боковой полосы производится сменой частоты
// второго гетеродина для режимов MODE_DOUBLE_IF_USB/LSB, или сменой частоты третьего гетеродина MODE_DOUBLE_IF
// в режиме MODE_DOUBLE_IF второй гетеродин выше первой ПЧ
//#define MODE_DOUBLE_IF
//#define MODE_DOUBLE_IF_USB
//#define MODE_DOUBLE_IF_LSB

// режим аналогичен MODE_DOUBLE_IF но в режиме передачи 2й и 3й гетеродины комутируются,
// тоесть второй формируется на CLK2, а третий - на CLK1
// для трактов с двумя промежуточными частотами где необходимо переключение
// гетеродинов при смене RX/TX
//#define MODE_DOUBLE_IF_SWAP23
//#define MODE_DOUBLE_IF_USB_SWAP23
//#define MODE_DOUBLE_IF_LSB_SWAP23

// множители частоты на выходах. в случае необходимости получения на выводе 2/4 кратной
// частоты установить в соответствующее значение
const long CLK0_MULT = 1;
const long CLK1_MULT = 1;
const long CLK2_MULT = 1;

// частота второго/третьего гетеродина - на 300Hz ниже/выше от точки точки -3dB на нижнем/верхнем скате фильтра 
const long IFreq_USB = 9828200;
const long IFreq_LSB = 9831500;

// первая промежуточная частота для трактов с двойным преобразованием частоты
const long IFreqEx = 45000000;

void UpdateFreq() {

#ifdef MODE_DC
  vfo.set_freq(
    CLK0_MULT*(trx.state.VFO[trx.GetVFOIndex()] + (trx.RIT && !trx.TX ? trx.RIT_Value : 0)),
    0,
    0
  );
#endif

#ifdef MODE_DC_QUADRATURE
  vfo.set_freq_quadrature(
    trx.state.VFO[trx.GetVFOIndex()] + (trx.RIT && !trx.TX ? trx.RIT_Value : 0),
    0
  );
#endif

#ifdef MODE_SINGLE_IF_VFOSB
    long f = trx.state.VFO[trx.GetVFOIndex()] + (trx.RIT && !trx.TX ? trx.RIT_Value : 0);
    if (trx.state.sideband == LSB) {
      f+=IFreq_USB;
    } else {
      f = (IFreq_USB > f ? IFreq_USB-f : f-IFreq_USB);
    }
  vfo.set_freq(CLK0_MULT*f,0,0);
#endif

#ifdef MODE_SINGLE_IF
  vfo.set_freq( // инверсия боковой - гетеродин сверху
    CLK0_MULT*(trx.state.VFO[trx.GetVFOIndex()] + (trx.state.sideband == LSB ? IFreq_USB : IFreq_LSB) + (trx.RIT && !trx.TX ? trx.RIT_Value : 0)),
    CLK1_MULT*(trx.state.sideband == LSB ? IFreq_USB : IFreq_LSB),
    0
  );
#endif

#ifdef MODE_SINGLE_IF_RXTX
  long f = CLK1_MULT*(trx.state.sideband == LSB ? IFreq_USB : IFreq_LSB); // инверсия боковой - гетеродин сверху
  vfo.set_freq(
    CLK0_MULT*(trx.state.VFO[trx.GetVFOIndex()] + (trx.state.sideband == LSB ? IFreq_USB : IFreq_LSB) + (trx.RIT && !trx.TX ? trx.RIT_Value : 0)),
    trx.TX ? 0 : f,
    trx.TX ? f : 0
  );
#endif

#ifdef MODE_SINGLE_IF_SWAP
  // инверсия боковой - гетеродин сверху
  long f1 = CLK0_MULT*(trx.state.VFO[trx.GetVFOIndex()] + (trx.state.sideband == LSB ? IFreq_USB : IFreq_LSB) + (trx.RIT && !trx.TX ? trx.RIT_Value : 0));
  long f2 = CLK1_MULT*(trx.state.sideband == LSB ? IFreq_USB : IFreq_LSB);
  vfo.set_freq(
    trx.TX ? f2 : f1,
    trx.TX ? f1 : f2,
    0
  );
#endif

#ifdef MODE_DOUBLE_IF
  long IFreq = (trx.state.sideband == USB ? IFreq_USB : IFreq_LSB);
  vfo.set_freq(
    CLK0_MULT*(trx.state.VFO[trx.GetVFOIndex()] + IFreqEx + (trx.RIT && !trx.TX ? trx.RIT_Value : 0)),
    CLK1_MULT*(IFreqEx + IFreq),
    CLK2_MULT*(IFreq)
  );
#endif

#ifdef MODE_DOUBLE_IF_LSB
  vfo.set_freq(
    CLK0_MULT*(trx.state.VFO[trx.GetVFOIndex()] + IFreqEx + (trx.RIT && !trx.TX ? trx.RIT_Value : 0)),
    CLK1_MULT*(IFreq + (trx.state.sideband == LSB ? IFreq_LSB : -IFreq_LSB)),
    CLK2_MULT*(IFreq_LSB)
  );
#endif

#ifdef MODE_DOUBLE_IF_USB
  vfo.set_freq(
    CLK0_MULT*(trx.state.VFO[trx.GetVFOIndex()] + IFreqEx + (trx.RIT && !trx.TX ? trx.RIT_Value : 0)),
    CLK1_MULT*(IFreq + (trx.state.sideband == USB ? IFreq_USB : -IFreq_USB)),
    CLK2_MULT*(IFreq_USB)
  );
#endif

#ifdef MODE_DOUBLE_IF_SWAP23
  long IFreq = (trx.state.sideband == USB ? IFreq_USB : IFreq_LSB); 
  long f1 = CLK0_MULT*(trx.state.VFO[trx.GetVFOIndex()] + IFreqEx + (trx.RIT && !trx.TX ? trx.RIT_Value : 0));
  long f2 = CLK1_MULT*(IFreqEx + IFreq);
  long f3 = CLK2_MULT*(IFreq);
  vfo.set_freq(
    f1,
    trx.TX ? f3 : f2,
    trx.TX ? f2 : f3
  );
#endif

#ifdef MODE_DOUBLE_IF_LSB_SWAP23
  long f1 = CLK0_MULT*(trx.state.VFO[trx.GetVFOIndex()] + IFreqEx + (trx.RIT && !trx.TX ? trx.RIT_Value : 0));
  long f2 = CLK1_MULT*(IFreq + (trx.state.sideband == LSB ? IFreq_LSB : -IFreq_LSB));
  long f3 = CLK2_MULT*(IFreq_LSB);
  vfo.set_freq(
    f1,
    trx.TX ? f3 : f2,
    trx.TX ? f2 : f3
  );
#endif

#ifdef MODE_DOUBLE_IF_USB_SWAP23
  long f1 = CLK0_MULT*(trx.state.VFO[trx.GetVFOIndex()] + IFreqEx + (trx.RIT && !trx.TX ? trx.RIT_Value : 0));
  long f2 = CLK1_MULT*(IFreq + (trx.state.sideband == USB ? IFreq_USB : -IFreq_USB));
  long f3 = CLK2_MULT*(IFreq_USB);
  vfo.set_freq(
    f1,
    trx.TX ? f3 : f2,
    trx.TX ? f2 : f3
  );
#endif
}

void UpdateBandCtrl() 
{
  // 0-nothing; 1-ATT; 2-Preamp
  switch (trx.state.AttPre) {
    case 0:
      outBandCtrl.Set(pnAtt,false);
      outBandCtrl.Set(pnPre,false);
      break;
    case 1:
      outBandCtrl.Set(pnAtt,true);
      outBandCtrl.Set(pnPre,false);
      break;
    case 2:
      outBandCtrl.Set(pnAtt,false);
      outBandCtrl.Set(pnPre,true);
      break;
  }
  outBandCtrl.Set(pnBand0, trx.BandIndex & 0x1);
  outBandCtrl.Set(pnBand1, trx.BandIndex & 0x2);
  outBandCtrl.Set(pnBand2, trx.BandIndex & 0x4);
  outBandCtrl.Set(pnBand3, trx.BandIndex & 0x8);
  outBandCtrl.Write();
}

#include "menu.h"

void loop()
{
  static long menu_tm = -1;
  bool tune = inTune.Read();
  trx.TX = tune || inTX.Read();
  trx.ChangeFreq(encoder.GetDelta());
  int keycode;
  if ((keycode=keypad.GetLastCode()) >= 0) {
    if (KeyMap[keycode & 0xF][keycode >> 4] == cmdLock && menu_tm >= 0 && millis()-menu_tm >= MENU_DELAY) {
      // отменяем команду
      trx.ExecCommand(cmdLock);
      // call to menu
      ShowMenu();
      // перерисовываем дисплей
      disp.clear();
      disp.reset();
      disp.Draw(trx);
      menu_tm = -1;
      return;
    }
  } else {
    menu_tm = -1; 
  }
  if ((keycode=keypad.Read()) >= 0) {
    TRXCommand cmd=KeyMap[keycode & 0xF][keycode >> 4];
    if (cmd == cmdLock) {
      // длительное нажатие MENU_KEY - вызов меню
      if (menu_tm < 0) {
        menu_tm = millis();
      }
    }
    trx.ExecCommand(cmd);
  }
  if (trx.RIT)
    trx.RIT_Value = (long)inRIT.ReadRaw()*2*RIT_MAX_VALUE/1024-RIT_MAX_VALUE;
  UpdateFreq();
  outQRP.Write(trx.QRP || tune);
  outTone.Write(tune);
  outTX.Write(trx.TX);
  UpdateBandCtrl();
  // read and convert smeter
  int val = inSMeter.Read();
  trx.SMeter =  0;
  for (byte i=14; i >= 0; i--) {
    if (val > SMeterMap[i]) {
      trx.SMeter =  i+1;
      break;
    }
  }
  // refresh display
  disp.Draw(trx);
}
