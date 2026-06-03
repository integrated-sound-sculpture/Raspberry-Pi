#ifndef INPUT_CONTROL_H
#define INPUT_CONTROL_H

const int calibration_button = 5;
const int ampl_slider_Pin = 4;
const int wave_select_pin = 2;
const int autotune_switch = 12;

void init_input_control();
void freq_calibration(int* freq_high, int* freq_low);
int pot_meter();
bool autotune_select();
float autotune(float freq);
#endif