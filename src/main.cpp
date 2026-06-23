#include <Arduino.h>
#include <cmath>
#include <pulse_cnt.h>
#include <input_control.h>
#include <iostream>
#include <chrono>

// UART2 pin definitions for communication with microprocessor
#define RXD2 16
#define TXD2 17

// Calibration constants
double A;
double B;

// Moving average filter variables
float samples[100] = {0};   // Buffer storing last 100 frequency measurements
int indx = 0;         // Current index in circular buffer
float sum = 0;        // Running sum of all samples

// Measurement variables
int in_freq;          // Raw input frequency from sensor
float freq;           // Output audio frequency
float d;              // Calculated hand distance
int amplitude;        // Volume/amplitude value from potentiometer

// Auto-tune selection flag
bool auto_tune;

void setup()
{
    // Initialize serial monitor
    Serial.begin(115200);

    // Initialize UART2 communication
    Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);


    // Initialize frequency measurement hardware
    init_frequencyMeter();

    // Initialize buttons, switches and potentiometers
    init_input_control();

    Serial.println("\nPress the button to input the frequency at 5 cm\n");

    // Number of calibration points
    int n = 4;

    // Known distances used for calibration (meters)
    float d[] = {0.05, 0.10, 0.20, 0.25};

    // Array to store measured frequencies
    float f[n];

    // Measure frequencies at the known distances
    freq_calibration2(f, n);

    //float f[] = {520000,560000,600000,610000};


    // Fit the frequency-distance model:
    fitModel(d, f, n, &A, &B);

    Serial.println("\nThe curve has been fitted A and B are:");
    Serial.println(A, 6);
    Serial.println(B, 6);

    Serial.println("\nSetup done");
    Serial.println("STARTING WITH MEASUREMENTS");


    
    delay(1500);
}

void loop()
{


    // Measure frequency from sensor
    in_freq = freq_measurement();
    Serial.print(in_freq);

    //--------------------------------------------------
    // Moving average filter (100 samples)
    //--------------------------------------------------

    // Remove oldest sample from running sum
    sum -= samples[indx];

    // Store new sample
    samples[indx] = in_freq;

    // Move to next position in circular buffer
    indx = (indx + 1) % 100;

    // Add new sample to running sum
    sum += in_freq;

    // Compute filtered frequency
    in_freq = sum / 100;

    Serial.print("               ");
    Serial.print(in_freq);

    //--------------------------------------------------
    // Convert frequency to hand distance
    //--------------------------------------------------

    d = convert_hand_lin(A, B, in_freq);
    //Serial.print("      distance:  ");
    //Serial.println(d);

    if (d > 0.30){
        d = 0.30;
    }
    

    //--------------------------------------------------
    // Convert distance to audible frequency
    //--------------------------------------------------
    /*
     * Parameters:
     * 4      = octave range
     * 130.8  = lowest note frequency (C3)
     * d      = measured hand distance
     * 30     = maximum distance (cm)
     * 5      = minimum distance (cm)
     */
    freq = convert_freq_log(3, 1046.5, d, 0.30, 0.05);

    if (freq > 1500){
        freq = 0.30;
    }
    
    //--------------------------------------------------
    // Read volume control potentiometer
    //--------------------------------------------------
    amplitude = pot_meter();

    //--------------------------------------------------
    // Auto-tune option
    //--------------------------------------------------
    auto_tune = autotune_select();

    if (auto_tune)
    {
        // Snap frequency to nearest musical note
        freq = autotune(freq);
    }

    //--------------------------------------------------
    // Read waveform selection
    //--------------------------------------------------
    /*
     * Possible values:
     * 0 = Sine
     * 1 = Square
     * 2 = Sawtooth (depending on implementation)
     */
    int waveform = waveform_select();

    //--------------------------------------------------
    // Read visualization mode
    //--------------------------------------------------
    int visualization = visualization_select();

    //--------------------------------------------------
    // Create UART packet
    //--------------------------------------------------
    /*
     * Format:
     * frequency,amplitude,waveform
     *
     * Example:
     * 440.00,128,0
     */
    String UART_data =
        String(freq) + "," +
        String(amplitude) + "," +
        String(waveform) + "," + 
        String(visualization) + "," +
        String(auto_tune) + "\n";

    // Send packet to connected device
    Serial2.println(UART_data);

    Serial.print("                ");
    Serial.println(UART_data);
    delay(1);
 

    
    if (digitalRead(5) == HIGH){
    
        Serial.println("           ");
        Serial.println("           ");
        Serial.println("           ");
        Serial.println("     Calibration      ");
        Serial.println("           ");
        Serial.println("           ");

        Serial2.println("Calibrate 1");
        Serial2.println(freq);

        delay(1000);
        // Number of calibration points
        int n = 4;

        // Known distances used for calibration (meters)
        float d[] = {0.05, 0.10, 0.20, 0.30};

        // Array to store measured frequencies
        float f[n];


        freq_calibration2(f, n);


        //float f[] = {520000,560000,600000,610000};


        // Fit the frequency-distance model:
        fitModel(d, f, n, &A, &B);

        delay(100);
    }

}