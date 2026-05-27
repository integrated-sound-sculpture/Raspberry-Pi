#include <Arduino.h>

#include <pulse_cnt.h>
//#include <audio.h>
#include <sine_gen.h>
#include <input_control.h>

#define RXD2 16
#define TXD2 17

int freq_high = 450000;
int freq_low = 400000;

void setup()
{
    Serial.begin(115200);

    Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2); //init UART

    init_frequencyMeter();
    //init_gen_wave(1200,8,SINE_WAVE);
    // audio.setup();
    //audio.enableAudioBeacon();

    Serial.println("\nPress the button to input the highest frequency\n");
    init_input_control();


    //freq_calibration(&freq_high, &freq_low);

    Serial.println("\nSetup done");
    Serial.println("STARTING WITH MEASUREMENTS");
    delay(3500);
}

void loop()
{
    int in_freq = freq_measurement();
    in_freq = 430000;
    float frequency = convert_freq_log(5,65.406, in_freq, freq_high, freq_low);
    int amplitude = pot_meter();  
    amplitude = 3000;

    String UART_data = String(frequency) + "," + String(amplitude) + "\n";
    Serial2.println(UART_data);

    Serial.println(UART_data);
    delay(300);
}