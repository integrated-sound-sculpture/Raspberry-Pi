#ifndef AUDIO_H
#define AUDIO_H

#include <Arduino.h>
#include <driver/dac.h>
#include <bitset>

typedef enum CarrierWave{
    BLOCK,
    CHIRP,
    SINE
} CarrierWave;

#define LUTSIZE 4096
#define BLOCKSIZE 2
#define SINESIZE 100
#define CODESIZE 32
#define MAX_DAC_FREQ 40000

const uint8_t blockLookupTable[] = {
    255, 0};

const uint8_t sineLookupTable[] = {
    128, 136, 143, 151, 159, 167, 174, 182,
    189, 196, 202, 209, 215, 220, 226, 231,
    235, 239, 243, 246, 249, 251, 253, 254,
    255, 255, 255, 254, 253, 251, 249, 246,
    243, 239, 235, 231, 226, 220, 215, 209,
    202, 196, 189, 182, 174, 167, 159, 151,
    143, 136, 128, 119, 112, 104, 96, 88,
    81, 73, 66, 59, 53, 46, 40, 35,
    29, 24, 20, 16, 12, 9, 6, 4,
    2, 1, 0, 0, 0, 1, 2, 4,
    6, 9, 12, 16, 20, 24, 29, 35,
    40, 46, 53, 59, 66, 73, 81, 88,
    96, 104, 112, 119};

class Audio {
    public:
        int fc = 10000;
        int fb = 5000;
        int fr = 32;
        int res;
        int skip;
        bool dacUse = false;
        CarrierWave carrierLUT = SINE;

        uint8_t lookupTable[LUTSIZE];


        // Timer0 Configuration Pointer (Handle)
        hw_timer_t *Timer0_Cfg = NULL;
        hw_timer_t *Timer1_Cfg = NULL;
        hw_timer_t *Timer3_Cfg = NULL;

        // Sine LookUpTable & Index Variable
        uint32_t SampleIdx = 0;
        uint8_t bitIdx = 0;
        // calculate greatest common devider for two numbers (Euclidian algorithm)
        int GCD(int i, int j);
        // calculate lowest common multiple for two numbers |i*j|/GCD(i,j)
        double LCM(int i, int j);

        void makeLookupTable(double t0);
        void setAudioBeacon();
        void enableAudioBeacon();
        void disableAudioBeacon();
        // void IRAM_ATTR Timer0_ISR();
        // void IRAM_ATTR Timer1_ISR();
        // void IRAM_ATTR Timer3_ISR();

        void setup();
};

extern Audio audio;

#endif