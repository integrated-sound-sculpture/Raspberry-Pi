#include <Arduino.h>
#include <pulse_cnt.h>
#include <audio.h>


void setup()
{
    init_frequencyMeter();
    audio.setup();
    audio.enableAudioBeacon();
}

void loop()
{
    int freq = freq_measurement();
}

