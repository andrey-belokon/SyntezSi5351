#include <avr/eeprom.h> 
#include "Encoder.h"
#include "Keypad_I2C.h"
#include "pins.h"
#include <string.h>
#include "TRX.h"
#include "disp_1602.h"
#include "si5351a.h"
#include "i2c.h"

// мапинг сканкодов на команды
const TRXCommand KeyMap[4][4] = {
  cmdBandUp, cmdBandDown, cmdAttPre, cmdVFOSel,
  cmdVFOEQ,  cmdUSBLSB,   cmdLock,   cmdSplit,
  cmdRIT,    cmdHam,      cmdZero,   cmdQRP,
  cmdNone,   cmdNone,     cmdNone,   cmdNone
};

KeypadI2C keypad(0x26);
Encoder encoder(360);
Display_1602_I2C disp(0x27);
TRX trx;

#define SI5351_XTAL_FREQ 270000000  // 0.1Hz resolution (10x mutiplier)
long EEMEM SI5351_XTAL_CORR = 0;
Si5351 vfo;

InputPullUpPin inTX(4);
InputPullUpPin inTune(5);
InputAnalogPin inRIT(A6,0,-1000,1000);
InputAnalogPin inSMetr(A7,0,0,1000);
OutputPin outTX(6,false,HIGH);
OutputPin outQRP(7,false,HIGH);
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

#include "Setup.h"

void setup()
{
  long correction;
  i2c_init();
  eeprom_read_block(&correction, &SI5351_XTAL_CORR, sizeof(correction));
  //Serial.begin(9600);
  //Serial.print(SI5351_XTAL_val);
  vfo.setup();
  vfo.set_xtal_freq(SI5351_XTAL_FREQ+correction);
  encoder.setup();
  keypad.setup();
  inTX.setup();
  inTune.setup();
  inRIT.setup();
  outTX.setup();
  outQRP.setup();
  outTone.setup();
  disp.setup();
  if (VFO_CalibrationMenu(vfo,disp,keypad,encoder,SI5351_XTAL_FREQ,correction)) {
    eeprom_write_block(&correction, &SI5351_XTAL_CORR, sizeof(correction));
    delay(2000);
  }
  trx.SwitchToBand(1);
}

// необходимо раскоментировать требуемую моду (только одну!)

// режим прямого преобразования. частота формируется на 1ом выводе. установить
// CLK0_MULT в значение 1/2/4 в зависимости от коэффициента делания частоты гетеродина
// второй и третий гетеродины отключены
//#define MODE_DIRECT_CONVERSION

// одна промежуточная частота. требуемая боковая формируется на счет переключения
// первого гетеродина "сверху" (IF=VFO-Fc) или "снизу" (IF=Fc-VFO)
// второй и третий гетеродины отключены. 
// режим предназначен для трактов где второй гетеродин кварцованый на фиксированную чатсоту
//#define MODE_SINGLE_IF_VFOSB

// одна промежуточная частота. первый гетеродин на выходе CLK0 всегда "сверху" (IF=VFO-Fc)
// требуемая боковая формируется на счет переключения второго гетеродина на выходе CLK1
// третий гетеродин отключен
#define MODE_SINGLE_IF

// режим аналогичен MODE_SINGLE_IF но в второй гетеродин генерируется на CLK1 при RX и
// на CLK2 в режиме TX
// для трактов котрые имеют отдельные смесители для формирования/детектирования сигнала
//#define MODE_SINGLE_IF_RXTX

// режим аналогичен MODE_SINGLE_IF но в режиме передачи гетеродины комутируются,
// тоесть первый формируется на CLK1, а второй - на CLK0
// для трактов где необходимо переключение гетеродинов при смене RX/TX
// третий гетеродин отключен
//#define MODE_SINGLE_IF_SWAP

// две промежуточные частоты. гетеродины формируются 1й - CLK0, 2й - CLK1, 3й - CLK2
// первый гетеродин всегда "сверху". выбор боковой полосы производится сменой частоты
// второго гетеродина
//#define MODE_DOUBLE_IF

// режим аналогичен MODE_DOUBLE_IF но в режиме передачи 2й и 3й гетеродины комутируются,
// тоесть второй формируется на CLK2, а третий - на CLK1
// для трактов с двумя промежуточными частотами где необходимо переключение
// гетеродинов при смене RX/TX
//#define MODE_DOUBLE_IF_SWAP23

// множители частоты на выходах. в случае необходимости получения на выводе 2/4 кратной
// частоты установить в соответствующее значение
const long CLK0_MULT = 1;
const long CLK1_MULT = 1;
const long CLK2_MULT = 1;

// промежуточная частота
const long IFreq = 9000000;

// вторая промежуточная частота для трактов с двойным преобразованием частоты
const long IFreq2 = 6000000;

// боковая полоса фильтра - USB или LSB
const int FilterSideband = USB;

// сдвиг частоты второго гетеродина для инверсии полосы
// значение константы всегда положительное!
// для FILTER_USB считаем что второй гетеродин выставлен на нижний скат фильтра
// для инверсии боковой увеличиваем его частоту на SidebandFreqShift Hz чтобы он попал
// на верхний скат
// в случае FILTER_LSB второй гетеродин выставлен на верхний скат и для инверсии
// частота гетеродина уменьшается на SidebandFreqShift Hz чтобы он попал на
// нижний скат
const long SidebandFreqShift = 3000;

void UpdateFreq() {

#ifdef MODE_DIRECT_CONVERSION
  vfo.set_freq(
    CLK0_MULT*(trx.state.VFO[trx.GetVFOIndex()] + (trx.RIT && !trx.TX ? trx.RIT_Value : 0)),
    0,
    0
  );
#endif

#ifdef MODE_SINGLE_IF_VFOSB
    long f = trx.state.VFO[trx.GetVFOIndex()] + (trx.RIT && !trx.TX ? trx.RIT_Value : 0);
    if (trx.state.sideband != FilterSideband) {
      f+=IFreq;
    } else {
      f = (IFreq > f ? IFreq-f : f-IFreq);
    }
  vfo.set_freq(CLK0_MULT*f,0,0);
#endif

#ifdef MODE_SINGLE_IF
  vfo.set_freq(
    CLK0_MULT*(trx.state.VFO[trx.GetVFOIndex()] + IFreq + (trx.RIT && !trx.TX ? trx.RIT_Value : 0)),
    CLK1_MULT*(IFreq + (trx.state.sideband != FilterSideband ? 0 : (FilterSideband == USB ? SidebandFreqShift : -SidebandFreqShift))),
    0
  );
#endif

#ifdef MODE_SINGLE_IF_RXTX
  long f = CLK1_MULT*(IFreq + (trx.state.sideband != FilterSideband ? 0 : (FilterSideband == USB ? SidebandFreqShift : -SidebandFreqShift)));
  vfo.set_freq(
    CLK0_MULT*(trx.state.VFO[trx.GetVFOIndex()] + IFreq + (trx.RIT && !trx.TX ? trx.RIT_Value : 0)),
    trx.TX ? 0 : f,
    trx.TX ? f : 0
  );
#endif

#ifdef MODE_SINGLE_IF_SWAP
  long f1 = CLK0_MULT*(trx.state.VFO[trx.GetVFOIndex()] + IFreq + (trx.RIT && !trx.TX ? trx.RIT_Value : 0));
  long f2 = CLK1_MULT*(IFreq + (trx.state.sideband != FilterSideband ? 0 : (FilterSideband == USB ? SidebandFreqShift : -SidebandFreqShift)));
  vfo.set_freq(
    trx.TX ? f2 : f1,
    trx.TX ? f1 : f2,
    0
  );
#endif

#ifdef MODE_DOUBLE_IF
  vfo.set_freq(
    CLK0_MULT*(trx.state.VFO[trx.GetVFOIndex()] + IFreq + (trx.RIT && !trx.TX ? trx.RIT_Value : 0)),
    CLK1_MULT*(IFreq + (trx.state.sideband == FilterSideband ? IFreq2 : -IFreq2)),
    CLK2_MULT*(IFreq2)
  );
#endif

#ifdef MODE_DOUBLE_IF_SWAP23
  long f1 = CLK0_MULT*(trx.state.VFO[trx.GetVFOIndex()] + IFreq + (trx.RIT && !trx.TX ? trx.RIT_Value : 0));
  long f2 = CLK1_MULT*(IFreq + (trx.state.sideband == FilterSideband ? IFreq2 : -IFreq2));
  long f3 = CLK2_MULT*(IFreq2);
  vfo.set_freq(
    f1,
    trx.TX ? f3 : f2,
    trx.TX ? f2 : f3
  );
#endif
}

void UpdateBandCtrl() {
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

void loop()
{
  bool tune = inTune.Read();
  trx.TX = tune || inTX.Read();
  trx.ChangeFreq(encoder.GetDelta());
  int keycode = keypad.Read();
  if (keycode >= 0) {
    trx.ExecCommand(KeyMap[keycode & 0xF][keycode >> 4]);
  }
  UpdateFreq();
  outQRP.Write(trx.QRP || tune);
  outTone.Write(tune);
  outTX.Write(trx.TX);
  UpdateBandCtrl();
  trx.SMeter = inSMetr.Read() /100;
  disp.Draw(trx);
}
