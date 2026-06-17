#include <Arduino.h>
#include "stdio.h"  
#include "input_control.h"
#include "pulse_cnt.h"


int potValue = 0;
bool lastState = LOW;

void init_input_control(){    //init calibration button and autotune_switch
    pinMode(calibration_button, INPUT_PULLUP);
    pinMode(autotune_switch, INPUT_PULLUP);

}


void freq_calibration(int* freq_high, int* freq_low){  //calibrates the low and high frequencies
  bool calibrate_high_freq = false;
  bool calibrate_low_freq = false;
  bool currentState;

  while(calibrate_low_freq == false){                         // loop until button has been pressed twice
    currentState = digitalRead(calibration_button);         // read the button stat 
    int x = freq_measurement();                             // measure frequency so it is already enabled (is not necessary)

    if (lastState == HIGH && currentState == LOW) {         // detect press (pull down button) only detect when state changes
        if(calibrate_high_freq == false){                     // when high freq has not yet been calibrated 
            *freq_high = freq_measurement();                // measure high freq
            calibrate_high_freq = true;                     // set flag high 
            Serial.println("High frequency calibrated:");       
            Serial.println(*freq_high);
            delay(500);
            Serial.println("\nPress the button to input the lowest frequency\n");

        }
        else{                                               // if high frequencie has already been calibrated
            calibrate_low_freq = true;                      // set flag high, so the while stops
            *freq_low = freq_measurement();                 // measure the low freq
            Serial.println("Low Frequency calibrated");      
            Serial.println(*freq_low);
        }
    }

    lastState = currentState;                              

    delay(100);         //delay to avoid jittering inputs from button

  }
}



void freq_calibration2(float freq[], int n){
    bool currentState;
    bool lastState = LOW;

    int frequency;

    float samples[50] = {0};   // Buffer storing last 100 frequency measurements
    int indx = 0;         // Current index in circular buffer
    float sum = 0;        // Running sum of all samples
    for (int i = 0; i<n; i++){

        while(true){ 

            frequency = freq_measurement();      
            
                // Remove oldest sample from running sum
            sum -= samples[indx];

            // Store new sample
            samples[indx] = frequency;

            // Move to next position in circular buffer
            indx = (indx + 1) % 50;

            //Serial.println(indx);
            // Add new sample to running sum
            sum += frequency;

            //Serial.println(sum);

            // Compute filtered frequency
            frequency = sum / 50;

            //Serial2.println(frequency);
            
            // measure frequency so it is already enabled (is not necessary)
            currentState = digitalRead(calibration_button);   
            
            if (lastState == HIGH && currentState == LOW) {
                freq[i] = frequency;

                Serial.print("frequency calibrated: ");       
                Serial.println(freq[i]);

                Serial2.println("Calibrate 1");
                Serial2.println(freq[i]);

                delay(300);
                Serial.println("\nPress the button to input the next frequency\n");
                lastState = currentState;
                break;
            }

            delay(20);
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
    if(potValue < 350){
        potValue = 0;
    }
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