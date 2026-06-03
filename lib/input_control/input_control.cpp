#include <Arduino.h>
#include "stdio.h"  
#include "input_control.h"
#include "pulse_cnt.h"


bool lastState = HIGH;
int potValue = 0;


void init_input_control(){    //init calibration button and autotune_switch
    pinMode(calibration_button, INPUT_PULLUP);
    pinMode(autotune_switch, INPUT_PULLUP);
}

void freq_calibration(int* freq_high, int* freq_low){  //calibrates the low and high frequencies
  bool calibrate_high_freq = false;
  bool calibrate_low_freq = false;
  bool currentState;

  while(calibrate_low_freq == LOW){                         // loop until button has been pressed twice
    currentState = digitalRead(calibration_button);         // read the button stat 
    int x = freq_measurement();                             // measure frequency so it is already enabled (is not necessary)

    if (lastState == HIGH && currentState == LOW) {         // detect press (pull down button) only detect when state changes
        if(calibrate_high_freq == LOW){                     // when high freq has not yet been calibrated 
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


bool autotune_select(){                                 // autotune selection returns bool when auto tune has been selected
    bool autotune = digitalRead(autotune_switch);
    delay(20);
    return autotune;
    //return false;
}

float autotune(float freq){                                //this function changes the a frequencie to a perfect note
    float note = 12 * std::log2((float) freq / 65.406);     // 1) change frequency to note this will be a decimal.
    int auto_note = std::round(note);                       // 2) round the note to nearest integer
    float auto_freq = 65.406 * pow(2,(float)auto_note/12);  // 3) convert note to frequency
    return auto_freq;
}

int pot_meter(){                                        // potmeter is used to change amplitude of the sound
    int potValue = analogRead(ampl_slider_Pin);         // tead the slider value (0-4096)
    return potValue;
}
 