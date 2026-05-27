#include <Arduino.h>
#include "DacTone.h"           // this is the DAC audio library
#include "sine_gen.h"


#define SQUARE_PIN 26 // Pin for square wave


//  triangular  GPIO25
//  sinusoidal  GPIO26
//  square      GPIO26


DacTone wave;                 // create audio object

void generateTriangleWave(int freq);

void init_gen_wave(int freq , int resolution, WaveType waveType){
  if(waveType != SINE_WAVE) {
    // Square wave configuration
    ledcAttachPin(SQUARE_PIN, 0);
    ledcWrite(0, 128); // 50% duty cycle
    ledcSetup(0, freq, resolution);

  }
}

void gen_wave(int freq, int resolution, WaveType waveType) {
    switch (waveType) {
      case SINE_WAVE:
        //while(freq<=4000){
          //freq +=10;
          wave.tone(freq);
          //Serial.println(freq);
        //}
       //while(freq>=80){
          //freq -=10;
          //wave.tone(freq);
          //Serial.println(freq);
       //}
        break;
      case SQUARE_WAVE:
        
        //while(freq<=4000){
          //freq +=1;
          //ledcSetup(0, freq, resolution);
          //Serial.println(freq);
          //delayMicroseconds(2);

        //}
       //while(freq>=80){
          //freq -=1;
        //ledcSetup(0, freq, resolution);
          //Serial.println(freq);
          //delayMicroseconds(2);
       //}
        break;
      case TRIANGLE_WAVE:
        generateTriangleWave(freq);
        break;
  }
}

void generateTriangleWave(int frequency) {
  ledcWrite(0, 0); // Turn off LEDC for DAC operation
  dac_output_enable(DAC_CHANNEL_1); // Enable DAC

  const int samples = 500; // Increase the number of samples per cycle to improve resolution
  int value = 0;
  int increment = 256 / (samples / 2);

  for (int i = 0; i < samples; ++i) {
    dac_output_voltage(DAC_CHANNEL_1, value);
    value += increment;
    if (value >= 256) {
      increment = -increment;
      value = 256 + increment;
    } else if (value <= 0) {
      increment = -increment;
      value = 0 + increment;
    }

    delayMicroseconds((1000000 / frequency) / samples);
  }
}


