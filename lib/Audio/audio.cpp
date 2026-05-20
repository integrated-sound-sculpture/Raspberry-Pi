#include "audio.h"

Audio audio;

// calculate greatest common devider for two numbers (Euclidian algorithm)
int Audio::GCD(int i, int j)
{
    int result;
    if (j == 0){
        result = i;
    }
    else{
        result = GCD(j, i % j);
    }
    return result;
}

// calculate lowest common multiple for two numbers |i*j|/GCD(i,j)
double Audio::LCM(int i, int j)
{
    double result;
    result = i*j/(double)GCD(i, j);
    return result;
}

void Audio::makeLookupTable(double t0)
{
    int cur_t0 = round(t0);

    switch (carrierLUT) {
        case BLOCK:
            for (int i=0; i<LUTSIZE; i++) {
                if (i % 2 == 0) {
                    lookupTable[i] = 255;
                }
                else {
                    lookupTable[i] = 0;
                }
            }
            break;
        case SINE:
            for (int i=0; i<LUTSIZE; i++) {
                lookupTable[i] = sineLookupTable[i % SINESIZE];
            }
            break;
        case CHIRP:
            for (int i=0; i<LUTSIZE; i++) {
                // chirp LUT
            }
            break;
        default:
            break;
    }
}

void Audio::setAudioBeacon()
{
    double timer_0;

    switch (carrierLUT) {
        case BLOCK:
            res = 2;
            break;
        case SINE:
        case CHIRP:
            res = SINESIZE;
            break;
        default:
            break;
    }
    skip = ceil((res*fc)/MAX_DAC_FREQ);
    Serial.print(skip);
    Serial.print(res*fc);
    lutSize = LUTSIZE/skip;

    if (res*fc/(double)skip > MAX_DAC_FREQ) {
        return;
    }
    else {
        timer_0 = skip/((double)res*(double)fc);
    }
    Serial.println(timer_0);
    timerAlarmWrite(Timer0_Cfg, (int)(timer_0*1000000), true);
}

void Audio::enableAudioBeacon()
{
    dacUse = true;
    timerRestart(Timer0_Cfg);
}

void Audio::disableAudioBeacon()
{
    dacUse = false;
}
// The Timer0 ISR Function (Executes Every Timer0 Interrupt Interval)
void IRAM_ATTR Timer0_ISR()
{
    switch(audio.carrierLUT) {
        case BLOCK:
            dac_output_voltage(DAC_CHANNEL_1, blockLookupTable[audio.SampleIdx++]);
            break;
        case SINE:
            dac_output_voltage(DAC_CHANNEL_1, sineLookupTable[(audio.SampleIdx++)*audio.skip]);
        default:
            dac_output_disable(DAC_CHANNEL_1);
    }
    if(audio.SampleIdx == audio.lutSize) {
        audio.SampleIdx = 0;
    }
}

void Audio::setup()
{
    // Configure Timer0 Interrupt
    Timer0_Cfg = timerBegin(0, 80, true);
    timerAttachInterrupt(Timer0_Cfg, &Timer0_ISR, true);
    timerAlarmWrite(Timer0_Cfg, 10000, true);
    timerAlarmEnable(Timer0_Cfg);
    timerStart(Timer0_Cfg);

    Serial.println("Timers set up");

    setAudioBeacon();
}