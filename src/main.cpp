#include <Arduino.h>

#include <pulse_cnt.h>
#include <audio.h>
#include <sine_gen.h>

void setup()
{
    Serial.begin(115200);
    //init_frequencyMeter();
    //init_gen_wave(300,8,SINE_WAVE);
    audio.setup();
    audio.enableAudioBeacon();
}

void loop()
{
   // int freq = freq_measurement();
   // gen_wave(1200,8,SINE_WAVE);
}

