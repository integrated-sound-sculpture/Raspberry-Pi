#include <Arduino.h>

#include <cmath>

#include <pulse_cnt.h>
//#include <sine_gen.h>
#include <input_control.h>


#define RXD2 16
#define TXD2 17

int freq_high = 610000;
int freq_low = 600000;
int in_freq;  //input frequency
float freq;
int amplitude;

bool auto_tune;   //auto tune select

void setup()
{
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2); //init UART

    dacWrite(25, 128);


    init_frequencyMeter();   //initialize the pulse counter
    init_input_control();    //initialize al the input pins for buttons/sliders

    Serial.println("\nPress the button to input the highest frequency\n");

    //freq_calibration(&freq_high, &freq_low);     //calibrate the high and low frequencies

    Serial.println("\nSetup done");               
    Serial.println("STARTING WITH MEASUREMENTS");
    delay(1500);
}

void loop()
{
    in_freq = freq_measurement();       //measure the input frequency
    Serial.print("input freq: ");
    Serial.println(freq);

    in_freq = 431000;
    freq = convert_freq_log(5,65.406, in_freq, freq_high, freq_low);   //convert the input frequency to audible frequencies

    amplitude = pot_meter();               //measure the amplitude using a potmeter
    amplitude = 3000;

    auto_tune = autotune_select();         //look if auto tune is selected
    if(auto_tune){
        freq = autotune(freq);   //tune the frequency to perfect notes
    }
   
   
    String UART_data = String(freq) + "," + String(amplitude) + "\n";    //make a UART package
    Serial2.println(UART_data);         //send data via UART connection
   
    Serial.println(UART_data);   

    delay(900);
}