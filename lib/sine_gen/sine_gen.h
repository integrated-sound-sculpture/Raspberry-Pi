#ifndef SINE_GEN_H
#define SINE_GEN_H
#include "DacTone.h"           // this is the DAC audio library


enum WaveType {
  SINE_WAVE,              // max 5 kHz
  SQUARE_WAVE,            // max 300 kHz
  TRIANGLE_WAVE           // max 300 Hz
};

void init_gen_wave(int freq, int resolution, WaveType waveType);
void gen_wave(int freq, int resolution, WaveType waveType);

#endif 