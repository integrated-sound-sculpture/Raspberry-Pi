#include <Arduino.h>
#include <pulse_cnt.h>


void setup()
{
    init_frequencyMeter();
}

void loop()
{
    int freq = freq_measurement();

}

