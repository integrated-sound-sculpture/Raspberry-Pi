#include <Arduino.h>
// BLOG Eletrogate
// ESP32 Frequency Meter
// ESP32 DevKit 38 pins + LCD
// https://blog.eletrogate.com/esp32-frequencimetro-de-precisao
// Rui Viana and Gustavo Murta august/2020

#include "stdio.h"                                                        // Library STDIO
#include "driver/pcnt.h"                                                  // Library ESP32 PCNT
#include "soc/pcnt_struct.h"
#include "pulse_cnt.h"
#include <cmath>




#define PCNT_COUNT_UNIT       PCNT_UNIT_0                                 // Set Pulse Counter Unit - 0 
#define PCNT_COUNT_CHANNEL    PCNT_CHANNEL_0                              // Set Pulse Counter channel - 0 

#define PCNT_INPUT_SIG_IO     GPIO_NUM_34                                 // Set Pulse Counter input - Freq Meter Input GPIO 34
#define PCNT_INPUT_CTRL_IO    GPIO_NUM_35                                 // Set Pulse Counter Control GPIO pin - HIGH = count up, LOW = count down  
#define OUTPUT_CONTROL_GPIO   GPIO_NUM_32                                 // Timer output control port - GPIO_32
#define PCNT_H_LIM_VAL        overflow                                    // Overflow of Pulse Counter 

#define IN_BOARD_LED          GPIO_NUM_2                                  // ESP32 native LED - GPIO 2

bool            flag          = true;                                     // Flag to enable print frequency reading
uint32_t        overflow      = 25000;                                    // Max Pulse Counter value
int16_t         pulses        = 0;                                        // Pulse Counter value
uint32_t        multPulses    = 0;                                        // Quantidade de overflows do contador PCNT
uint32_t        sample_time   = 20000;                                    // sample time of 1 second to count pulses
float           frequency     = 0;                                        // frequency value
char            buf[32];                                                  // Buffer

esp_timer_create_args_t create_args;                                      // Create an esp_timer instance
esp_timer_handle_t timer_handle;                                          // Create an single timer

portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;                     // portMUX_TYPE to do synchronism

//----------------------------------------------------------------------------------
static void IRAM_ATTR pcnt_intr_handler(void *arg)                        // Counting overflow pulses
{
  portENTER_CRITICAL_ISR(&timerMux);                                      // disabling the interrupts
  multPulses++;                                                           // increment Overflow counter
  PCNT.int_clr.val = BIT(PCNT_COUNT_UNIT);                                // Clear Pulse Counter interrupt bit
  portEXIT_CRITICAL_ISR(&timerMux);                                       // enabling the interrupts
}

//----------------------------------------------------------------------------------
void init_PCNT(void)                                                      // Initialize and run PCNT unit
{
  pcnt_config_t pcnt_config = { };                                        // PCNT unit instance

  pcnt_config.pulse_gpio_num = PCNT_INPUT_SIG_IO;                         // Pulse input GPIO 34 - Freq Meter Input
  pcnt_config.ctrl_gpio_num = PCNT_INPUT_CTRL_IO;                         // Control signal input GPIO 35
  pcnt_config.unit = PCNT_COUNT_UNIT;                                     // Unidade de contagem PCNT - 0
  pcnt_config.channel = PCNT_COUNT_CHANNEL;                               // PCNT unit number - 0
  pcnt_config.counter_h_lim = PCNT_H_LIM_VAL;                             // Maximum counter value - 20000
  pcnt_config.pos_mode = PCNT_COUNT_INC;                                  // PCNT positive edge count mode - inc
  pcnt_config.neg_mode = PCNT_COUNT_INC;                                  // PCNT negative edge count mode - inc
  pcnt_config.lctrl_mode = PCNT_MODE_DISABLE;                             // PCNT low control mode - disable
  pcnt_config.hctrl_mode = PCNT_MODE_KEEP;                                // PCNT high control mode - won't change counter mode
  pcnt_unit_config(&pcnt_config);                                         // Initialize PCNT unit

  pcnt_counter_pause(PCNT_COUNT_UNIT);                                    // Pause PCNT unit
  pcnt_counter_clear(PCNT_COUNT_UNIT);                                    // Clear PCNT unit

  pcnt_event_enable(PCNT_COUNT_UNIT, PCNT_EVT_H_LIM);                     // Enable event to watch - max count
  pcnt_isr_register(pcnt_intr_handler, NULL, 0, NULL);                    // Setup Register ISR handler
  pcnt_intr_enable(PCNT_COUNT_UNIT);                                      // Enable interrupts for PCNT unit

  pcnt_counter_resume(PCNT_COUNT_UNIT);                                   // Resume PCNT unit - starts count
}

//----------------------------------------------------------------------------------
void read_PCNT(void *p)                                                   // Read Pulse Counter
{
  gpio_set_level(OUTPUT_CONTROL_GPIO, 0);                                 // Stop counter - output control LOW
  pcnt_get_counter_value(PCNT_COUNT_UNIT, &pulses);                       // Read Pulse Counter value
  flag = true;                                                            // Change flag to enable print
}

//---------------------------------------------------------------------------------
void init_frequencyMeter()
{
  init_PCNT();                                                            // Initialize and run PCNT unit

  gpio_pad_select_gpio(OUTPUT_CONTROL_GPIO);                              // Set GPIO pad
  gpio_set_direction(OUTPUT_CONTROL_GPIO, GPIO_MODE_OUTPUT);              // Set GPIO 32 as output

  create_args.callback = read_PCNT;                                       // Set esp-timer argument
  esp_timer_create(&create_args, &timer_handle);                          // Create esp-timer instance

  gpio_set_direction(IN_BOARD_LED, GPIO_MODE_OUTPUT);                     // Set LED inboard as output

  gpio_matrix_in(PCNT_INPUT_SIG_IO, SIG_IN_FUNC226_IDX, false);           // Set GPIO matrin IN - Freq Meter input
  gpio_matrix_out(IN_BOARD_LED, SIG_IN_FUNC226_IDX, false, false);        // Set GPIO matrix OUT - to inboard LED
}

//----------------------------------------------------------------------------------------

int freq_measurement()
{
  if (flag == true)                                                     // If count has ended
  {
    flag = false;                                                       // change flag to disable print
    frequency = (pulses + (multPulses * overflow)) / 2.0;               // Calculation of frequency
    multPulses = 0;                                                     // Clear overflow counter

    pcnt_counter_clear(PCNT_COUNT_UNIT);                                // Clear Pulse Counter
    esp_timer_start_once(timer_handle, sample_time);                    // Initialize High resolution timer (1 sec)
    gpio_set_level(OUTPUT_CONTROL_GPIO, 1);                             // Set enable PCNT count
  }
  return frequency * (1/0.02);
}


//---------------------------------------------------------------------------------

float convert_freq_lin(int in_min, int in_max, int out_min, int out_max, int freq){        //converting the frequency using a linear scale
  float a = (float)(in_max-in_min)/(float)(out_max-out_min);
  float b = in_min - a*out_min;
  float y = a*freq +b;
  return y;
}


//---------------------------------------------------------------------------------

float convert_freq_log(int num_octaves, float min_freq, float freq, float in_max, float in_min){    //converting teh frequency using a logaritmic scale

  float x = (float)(freq-in_min)/(float)(in_max-in_min); //convert input freq to range between 0 and 1
  
  //Serial.println(x);
  float y = min_freq * pow(2,x*num_octaves);  //convert to logaritmic range 
  return y;
}


//---------------------------------------------------------------------------------

// Converts a measured frequency into a hand distance.
//
// The formula is derived from the fitted sensor model:
//
//      f = A / sqrt(1 + B/d)
//
// Rearranging for distance d gives:
//
//      d = B / (A²/f² - 1)
//
// Parameters:
//   A, B  -> calibration constants obtained from fitModel()
//   freq  -> measured sensor frequency
//
// Returns:
//   Estimated hand distance
//

float convert_hand_lin(float A, float B, int freq)
{
    float d = B / (std::pow(A, 2) / std::pow(freq, 2) - 1);
    return d;
}

//---------------------------------------------------------------------------------

// Fits the sensor model:
//
//      1/f² = (1/A²) + (B/A²)(1/d)
//
// This transforms the nonlinear relationship into a linear form:
//
//      Y = c + mX
//
// where:
//
//      X = 1/d
//      Y = 1/f²
//      c = 1/A²
//      m = B/A²
//
// Linear regression is then used to determine m and c,
// from which A and B are recovered.
//

void fitModel(float d[], float f[], int n, double* A, double* B)
{
    // Variables used to compute least-squares regression
    double sumX  = 0.0f;   // ΣX
    double sumY  = 0.0f;   // ΣY
    double sumXX = 0.0f;   // ΣX²
    double sumXY = 0.0f;   // ΣXY

    // Build regression sums from calibration points
    for (int i = 0; i < n; i++)
    {
        // Ignore invalid calibration points
        if (d[i] <= 0 || f[i] <= 0)
            continue;

        // Linearized variables
        float X = 1.0f / d[i];
        float Y = 1.0f / (f[i] * f[i]);

        // Accumulate regression sums
        sumX  += X;
        sumY  += Y;
        sumXX += X * X;
        sumXY += X * Y;
    }

    // Denominator used in least-squares formulas
    float denom = n * sumXX - sumX * sumX;

    // Calculate slope (m) of best-fit line
    float m = (n * sumXY - sumX * sumY) / denom;

    // Calculate intercept (c) of best-fit line
    float c = (sumY - m * sumX) / n;

    // Recover model parameter A
    *A = 1.0f / sqrtf(c);

    // Recover model parameter B
    *B = m / c;
}