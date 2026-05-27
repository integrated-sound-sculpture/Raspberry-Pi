#ifndef INPUT_CONTROL_H
#define INPUT_CONTROL_H

const int calibration_button = 5;
const int potPin = 4;



void init_input_control();
void freq_calibration(int* freq_high, int* freq_low);
int pot_meter();
#endif