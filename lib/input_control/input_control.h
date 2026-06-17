#ifndef INPUT_CONTROL_H
#define INPUT_CONTROL_H

const int calibration_button = 5;
const int ampl_slider_Pin = 4;
const int wave_select_pin = 2;
const int autotune_switch = 12;
const int waveform_pin1 = 22;
const int waveform_pin2 = 23;
const int vis_select1 = 18;
const int vis_select2 = 19;

void init_input_control();
void freq_calibration(int* freq_high, int* freq_low);
int pot_meter();
bool autotune_select();
float autotune(float freq);
int waveform_select();
int visualization_select();

void freq_calibration2(float freq[], int n);


#endif