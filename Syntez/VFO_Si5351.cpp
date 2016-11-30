#include "VFO_Si5351.h"

void VFO_Si5351::setup(long xtal_freq, long correction,
  enum si5351_drive CLK0_Drive,
  enum si5351_drive CLK1_Drive,
  enum si5351_drive CLK2_Drive) {
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, xtal_freq);
  si5351.set_correction(correction);
  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
  si5351.drive_strength(SI5351_CLK0,CLK0_Drive);
  si5351.drive_strength(SI5351_CLK1,CLK1_Drive);
  si5351.drive_strength(SI5351_CLK2,CLK2_Drive);
  si5351.output_enable(SI5351_CLK0, 0);
  si5351.output_enable(SI5351_CLK1, 0);
  si5351.output_enable(SI5351_CLK2, 0);
}

void VFO_Si5351::SetCorrection(long correction) {
  si5351.set_correction(correction);
  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
}

void VFO_Si5351::OutCalibrationFreq(long freq) {
  si5351.set_freq(freq*SI5351_FREQ_MULT, SI5351_PLL_FIXED, SI5351_CLK0);
}

void VFO_Si5351::SetFreq(long f0, long f1, long f2) {
  if (f0 != freq0) {
  	if (f0) {
  	  if (freq0 == 0) {
  		  si5351.output_enable(SI5351_CLK0, 1);
  	  }
  	  si5351.set_freq(f0*SI5351_FREQ_MULT, SI5351_PLL_FIXED, SI5351_CLK0);
  	} else {
  	  si5351.output_enable(SI5351_CLK0, 0);
  	}
  	freq0 = f0;
  }
  if (f1 != freq1) {
  	if (f1) {
  	  if (freq1 == 0) {
  		  si5351.output_enable(SI5351_CLK1, 1);
  	  }
  	  si5351.set_freq(f1*SI5351_FREQ_MULT, SI5351_PLL_FIXED, SI5351_CLK1);
  	} else {
  	  si5351.output_enable(SI5351_CLK1, 0);
  	}
  	freq1 = f1;
  }
  if (f2 != freq2) {
  	if (f2) {
  	  if (freq2 == 0) {
  		  si5351.output_enable(SI5351_CLK2, 1);
  	  }
  	  si5351.set_freq(f2*SI5351_FREQ_MULT, SI5351_PLL_FIXED, SI5351_CLK2);
  	} else {
  	  si5351.output_enable(SI5351_CLK2, 0);
  	}
  	freq2 = f2;
  }
}


