#include <Arduino.h>
#include <pulse_cnt.h>
#include <audio.h>


void setup()
{
    Serial.begin(115200);
    // init_frequencyMeter();
    audio.setup();
    audio.enableAudioBeacon();
}

void loop()
{
    // int freq = freq_measurement();
}

