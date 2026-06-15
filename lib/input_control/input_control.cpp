#include <Arduino.h>
#include "stdio.h"  
#include "input_control.h"
#include "pulse_cnt.h"


int potValue = 0;


void init_input_control(){    //init calibration button and autotune_switch
    pinMode(calibration_button, INPUT_PULLUP);
    pinMode(autotune_switch, INPUT_PULLUP);

}


void freq_calibration2(float freq[], int n){
    bool currentState;
    bool lastState = LOW;
    for (int i = 0; i<n; i++){

        while(true){ 

            int x = freq_measurement();                            // measure frequency so it is already enabled (is not necessary)
            currentState = digitalRead(calibration_button);   
            
            if (lastState == HIGH && currentState == LOW) {
                freq[i] = freq_measurement();

                Serial.print("frequency calibrated: ");       
                Serial.println(freq[i]);

                Serial2.println("Calibrate 1");
                Serial2.println(freq[i]);

                delay(300);
                Serial.println("\nPress the button to input the next frequency\n");
                lastState = currentState;
                break;

            }

            lastState = currentState;

        }
    }
}



bool autotune_select(){                                 // autotune selection returns bool when auto tune has been selected
    bool autotune = digitalRead(autotune_switch);
    return autotune;
}

float autotune(float freq){                                //this function changes the a frequency to a perfect note
    float note = 12 * std::log2((float) freq / 65.406);     // 1) change frequency to note this will be a decimal.
    int auto_note = std::round(note);                       // 2) round the note to nearest integer
    float auto_freq = 65.406 * pow(2,(float)auto_note/12);  // 3) convert note to frequency
    return auto_freq;
}

int pot_meter(){                                        // potmeter is used to change amplitude of the sound
    int potValue = analogRead(ampl_slider_Pin);         // read the slider value (0-4096)
    return potValue;
}
 

int waveform_select(){
    bool waveform_select1 = digitalRead(waveform_pin1);
    bool waveform_select2 = digitalRead(waveform_pin2);
    int waveform = waveform_select1 + 2 * waveform_select2;
    return waveform;
}


int visualization_select(){
    bool vis_select1 = digitalRead(vis_select1);
    bool vis_select2 = digitalRead(vis_select2);
    int visualization= vis_select1 + 2 * vis_select2;
    return visualization;
}