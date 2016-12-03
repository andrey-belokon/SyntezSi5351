#ifndef TRX_H
#define TRX_H

#if ARDUINO < 100
#include <WProgram.h>
#else
#include <Arduino.h>
#endif

#define LSB 0
#define USB 1

// число диапазонов
#define BAND_COUNT 9

const struct {
  int   mc;
  long  start, startSSB, end;
  byte sideband;
} Bands[BAND_COUNT] = {
  {160, 1810000L, 1840000L, 2000000L, LSB},
  {80,  3500000L, 3600000L, 3800000L, LSB},
  {40,  7000000L, 7045000L, 7200000L, LSB},
  {30,  10100000L, 0, 10150000L, USB},
  {20,  14000000L, 14100000L, 14350000L, USB},
  {17,  18068000L, 18110000L, 18168000L, USB},
  {15,  21000000L, 21150000L, 21450000L, USB},
  {12,  24890000L, 24930000L, 25140000L, USB},
  {10,  28000000L, 28200000L, 29700000L, USB}
};

// для режима general coverage
#define FREQ_MIN  1000000L
#define FREQ_MAX 30000000L

// список комманд трансивера
typedef enum {
  cmdBandUp,   // переключение диапазонов или частоты
  cmdBandDown,
  cmdAttPre,   // переключает по кругу аттенюатор/увч
  cmdVFOSel,   // VFO A/B
  cmdVFOEQ,    // VFO A=B
  cmdUSBLSB,   // выбор боковой USB/LSB
  cmdLock,     // Lock freq
  cmdSplit,    // Split on/off
  cmdRIT,      // RIT
  cmdHam,      // режим Ham band/General coverage. в режиме Ham кнопки cmdBandUp/Down переключают диапазоны
  // в режиме General coverage - изменяют частоту на +/-1MHz
  cmdZero,     // устанавливает частоту точно по еденицам кГц. 3623145->3623000
  cmdQRP,     // режим уменьшенной выходной мощности
  cmdNone
} TRXCommand;

// состояние VFO для диапазона
typedef struct {
  long VFO[2];    // VFO freq A&B
  int  VFO_Index; // 0-A, 1-B
  byte sideband;
  byte AttPre;    // 0-nothing; 1-ATT; 2-Preamp
  bool Split;
} TVFOState;

class TRX {
  public:
	  TVFOState BandData[BAND_COUNT];
	  int BandIndex;  // -1 в режиме General coverage
    TVFOState state;
    bool TX;
    bool Lock;
    bool RIT;
    int RIT_Value;
	  bool QRP;
	  byte SMeter; // 1..9

	  TRX();
    void SwitchToBand(int band);
    void ExecCommand(TRXCommand cmd);
    int GetVFOIndex() {
      return (state.Split && TX ? state.VFO_Index ^ 1 : state.VFO_Index);
    }
    void ChangeFreq(long freq_delta);
};

class TRXDisplay {
  public:
	  virtual void setup() {}
	  virtual void Draw(TRX& trx) {}
	  virtual void DrawCalibration(const char* title, long value, bool hi_res);
};

#endif
