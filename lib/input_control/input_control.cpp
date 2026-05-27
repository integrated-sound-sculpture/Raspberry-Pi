#include <Arduino.h>
#include "stdio.h"  
#include "input_control.h"
#include "pulse_cnt.h"


bool lastState = HIGH;
int potValue = 0;


void init_input_control(){
    pinMode(calibration_button, INPUT_PULLUP);

}

void freq_calibration(int* freq_high, int* freq_low){
  bool calibrate_high_freq = false;
  bool calibrate_low_freq = false;
  bool currentState;

  while(calibrate_low_freq == LOW){
    currentState = digitalRead(calibration_button);
    int x = freq_measurement();
    // Detect press
    if (lastState == HIGH && currentState == LOW) {
        if(calibrate_high_freq == LOW){
            *freq_high = freq_measurement();
            calibrate_high_freq = true;
            Serial.println("High frequency calibrated:");
            Serial.println(*freq_high);
            delay(500);
            Serial.println("\nPress the button to input the lowest frequency\n");

        }
        else{
            calibrate_low_freq = true;
            *freq_low = freq_measurement();
            Serial.println("Low Frequency calibrated");
            Serial.println(*freq_low);
        }
    }

    lastState = currentState;

    delay(100);

  }
}

int pot_meter(){
    int potValue = analogRead(potPin);
    return potValue;
}
 