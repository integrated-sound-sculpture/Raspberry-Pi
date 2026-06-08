#ifndef PULSE_CNT_H
#define PULSE_CNT_H

void init_frequencyMeter();
int freq_measurement();

float convert_freq_log(int num_octaves, float min_freq, float freq, int in_max, int in_min);
float convert_freq_lin(int in_min, int in_max, int out_min, int out_max, int freq);
float convert_hand_lin(int f_min, int f_max, int freq);
#endif 