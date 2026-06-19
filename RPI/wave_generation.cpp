/*
 * sound_generation.cxx
 * 
 * Copyright 2026  <pi@raspberrypi>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

#include <atomic>

#include <unistd.h>
#include <thread>
#include <chrono>

#include <fcntl.h>			//Used for UART
#include <termios.h>		//Used for UART

#include <stdio.h>
#include <stdlib.h>
#include <math.h>            // Required for sin()
#include <alsa/asoundlib.h>

#include <string.h>
#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <gpiod.h>

#define S0 17
#define S1 27
#define S2 22
#define S3 23
#define E0 25
#define E1 24
#define SD 26 //Shutdown pin

#define PORT        8080 
#define MAXLINE     1024 

#define WAVE_FORM BLOCK
#define SAMPLE_RATE 48000
#define BUFFER_SIZE 2048                // Smaller buffers provide faster frequency updates


typedef enum{
    SINE,
    BLOCK,
    TRIANGLE,
    FREEBIRD
}FORM;

typedef struct settings{
    // std::atomic<float> f;
    // std::atomic<FORM> wave;
    // std::atomic<float> ampl;
    float f;
    FORM wave;
    float ampl;
    uint8_t vis_sett;
}settings;

short buffer[BUFFER_SIZE];

settings f1{
    440.0f,
    WAVE_FORM,
    1.0f,
    1
};

double led_frequency = f1.f;

int sock = socket(AF_INET, SOCK_DGRAM, 0);
using std::chrono::high_resolution_clock;

// GPIO chip and request for libgpiod v2.x
struct gpiod_chip *gpiod_chip_handle = NULL;
struct gpiod_line_request *gpio_request = NULL;
unsigned int gpio_offsets[7] = {S0, S1, S2, S3, E0, E1, SD};

void init_gpio(void)
{
    // Open the GPIO chip (typically /dev/gpiochip0 on Raspberry Pi)
    gpiod_chip_handle = gpiod_chip_open("/dev/gpiochip0");
    if (!gpiod_chip_handle) {
        perror("gpiod_chip_open");
        exit(1);
    }

    // Create line configuration for all pins as outputs
    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    if (!line_cfg) {
        perror("gpiod_line_config_new");
        gpiod_chip_close(gpiod_chip_handle);
        exit(1);
    }

    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    if (!settings) {
        perror("gpiod_line_settings_new");
        gpiod_line_config_free(line_cfg);
        gpiod_chip_close(gpiod_chip_handle);
        exit(1);
    }

    // Configure settings as output
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    // gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_ACTIVE);

    // Add all pins with these settings
    if (gpiod_line_config_add_line_settings(line_cfg, gpio_offsets, 7, settings) < 0) {
        perror("gpiod_line_config_add_line_settings");
        gpiod_line_settings_free(settings);
        gpiod_line_config_free(line_cfg);
        gpiod_chip_close(gpiod_chip_handle);
        exit(1);
    }

    // Create request config
    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    if (!req_cfg) {
        perror("gpiod_request_config_new");
        gpiod_line_settings_free(settings);
        gpiod_line_config_free(line_cfg);
        gpiod_chip_close(gpiod_chip_handle);
        exit(1);
    }

    gpiod_request_config_set_consumer(req_cfg, "wave_generation");

    // Request the lines
    gpio_request = gpiod_chip_request_lines(gpiod_chip_handle, req_cfg, line_cfg);
    if (!gpio_request) {
        perror("gpiod_chip_request_lines");
        gpiod_request_config_free(req_cfg);
        gpiod_line_settings_free(settings);
        gpiod_line_config_free(line_cfg);
        gpiod_chip_close(gpiod_chip_handle);
        exit(1);
    }

    // Clean up temporary configs
    gpiod_request_config_free(req_cfg);
    gpiod_line_settings_free(settings);
    gpiod_line_config_free(line_cfg);
}

void cleanup_gpio(void)
{
    // Release the line request
    if (gpio_request) {
        gpiod_line_request_release(gpio_request);
    }

    // Close the chip
    if (gpiod_chip_handle) {
        gpiod_chip_close(gpiod_chip_handle);
    }
}

void drive_leds(void)
{
    uint8_t value = 0;
    int i;
    float f;
    enum gpiod_line_value array[6];

    while(true) {
        f = led_frequency;
        value = (uint8_t)round(8 * log2(f / 130.8));
        if (value < 0) {
            value = 0;
        } else if (value > 31) {
            value = 31;
        }

        for (i = 0; i < 5; i++) {
            array[i] = static_cast<enum gpiod_line_value>((value >> i) & 1);
        }
        array[5] = static_cast<enum gpiod_line_value>((~value >> 4) & 1);
        
        // Write values using libgpiod v2.x API
        gpiod_line_request_set_value(gpio_request, gpio_offsets[0], array[0]);  // S0
        gpiod_line_request_set_value(gpio_request, gpio_offsets[1], array[1]);  // S1
        gpiod_line_request_set_value(gpio_request, gpio_offsets[2], array[2]);  // S2
        gpiod_line_request_set_value(gpio_request, gpio_offsets[3], array[3]);  // S3
        gpiod_line_request_set_value(gpio_request, gpio_offsets[4], array[4]);  // E0
        gpiod_line_request_set_value(gpio_request, gpio_offsets[5], array[5]);  // E1 (inverted)

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

//-------------------------
//----- SETUP USART 0 -----
//-------------------------
//At bootup, pins 8 and 10 are already set to UART0_TXD, UART0_RXD (ie the alt0 function) respectively
int uart0_filestream = -1;

struct termios options;

void set_interface_attribs(void)
{
    //Open uart
    uart0_filestream = open("/dev/ttyS0", O_RDWR | O_NOCTTY);		//Open in non blocking read/write mode
    if (uart0_filestream == -1) {
        //ERROR - CAN'T OPEN SERIAL PORT
        printf("Error: Unable to open UART\n");
    }

    //Configure uart
    
    tcgetattr(uart0_filestream, &options);

    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);

    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;

    options.c_lflag = 0;
    options.c_oflag = 0;
    options.c_iflag = IGNPAR;

    options.c_cc[VMIN]  = 1;
    options.c_cc[VTIME] = 1;

    tcsetattr(uart0_filestream, TCSANOW, &options);
}

bool read_line(int fd, char* buffer, size_t maxlen)
{
    size_t idx = 0;

    while (idx < maxlen - 1)
    {
        char c;

        int n = read(fd, &c, 1);

        if (n > 0)
        {
            // newline reached
            if (c == '\n')
            {
                break;
            }

            // ignore carriage return
            if (c != '\r')
            {
                buffer[idx++] = c;
            }
        }
    }

    buffer[idx] = '\0';

    return idx > 0;
}

void parameter_aquisition(void)
{
    while(true){
        //----- CHECK FOR ANY RX BYTES -----
        if (uart0_filestream != -1)
        {
            char line[128];

            if(read_line(uart0_filestream, line, sizeof(line)))
            {
                float freq;
                float ampl;
                int form;
                int setting;
                int calib;
                
                printf("RX = [%s]\n", line);
                // printf("%d\n", sscanf(line, "%f,%d,%d", &freq, &ampl, &form) == 3);

                if(sscanf(line, "%f,%f,%d,%d", &freq, &ampl, &form, &setting) == 4)
                {
                    // printf("Hello");
                    f1.f = freq;
                    f1.wave = static_cast<FORM>(form);
                    f1.ampl = ampl/4096;
                    f1.vis_sett = setting;
                    // printf("Frequency: %.2f Hz, Amplitude: %.2f\n", f1.f, f1.ampl);
                }
                else if(sscanf(line, "Calibration %d", &calib) == 1)
                {
                    // Handle calibration command
                }
            }
        }
    }
}

void make_song_array(float *song, std::string *notes, int len_notes)
{
    int octave;
    int sharp;
    int n;
    char last;

    float super;
    float f0 = 130.8;

    for (int i=0; i < len_notes; i++)
    {
        if (notes[i].find("#") != std::string::npos) {
                sharp = 1;
        }
        else {
            sharp = 0;
        }

        last = notes[i].back();
        octave = last - '0';

        if (notes[i].find("C") != std::string::npos) {
            n = 0;
        }
        else if (notes[i].find("D") != std::string::npos) {
            n = 2;
        }
        else if (notes[i].find("E") != std::string::npos) {
            n = 4;
        }
        else if (notes[i].find("F") != std::string::npos) {
            n = 5;
        }
        else if (notes[i].find("G") != std::string::npos) {
            n = 7;
        }
        else if (notes[i].find("A") != std::string::npos) {
            n = 9;
        }
        else if (notes[i].find("B") != std::string::npos) {
            n = 11;
        }
        else {
            n = 0;
        }

        super = (float) n + sharp + 12 * (octave - 3);
        song[i] = f0 * pow(2, super / 12);
        printf("Note: %s, Frequency: %.2f Hz\n", notes[i].c_str(), song[i]);
    }
}

int main(void)
{
    // Keep speaker in shutdown until setup is done
    enum gpiod_line_value sd_pin_val = static_cast<enum gpiod_line_value>(0);
    gpiod_line_request_set_value(gpio_request, gpio_offsets[6], sd_pin_val);

    // UART
    set_interface_attribs();

    // GPIO
    init_gpio();
    
    // Subprocesses
    std::thread t0(parameter_aquisition);
    std::thread t1(drive_leds);

    t0.detach();
    t1.detach();
    
    // Communication to Python over static internal network
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Audio
    int err;
    snd_pcm_t *handle;
    // Open and configure the playback device
    // Use the default ALSA device so software conversion can handle the sample format/rate if needed.
    err = snd_pcm_open(&handle, "hw:CARD=Headphones", SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        fprintf(stderr, "Playback open error: %s\n", snd_strerror(err));
        cleanup_gpio();
        return EXIT_FAILURE;
    }

    err = snd_pcm_set_params(handle,
                             SND_PCM_FORMAT_S16_LE,
                             SND_PCM_ACCESS_RW_INTERLEAVED,
                             1, SAMPLE_RATE, 1, 40000);
    if (err < 0) {
        fprintf(stderr, "Hardware configuration error: %s\n", snd_strerror(err));
        snd_pcm_close(handle);
        cleanup_gpio();
        return EXIT_FAILURE;
    }

    printf("Reading frequency from uart and .\n");

    double phase = 0.0; // The critical running phase accumulator
    double current_frequency = f1.f;
    double current_amplitude = f1.ampl;
    FORM current_wave = f1.wave;
    int visualisation_select = f1.vis_sett;
    std::chrono::duration<double, std::milli> ms_double;
    auto t1t = high_resolution_clock::now();
    auto t2t = high_resolution_clock::now();

    bool start = true;

    double t = 0.0;
    double t_last = 0.0;

    int k = 0;
    // twinkle twinkle
    // std::string notes[] = {"C4", "C4", "G4", "G4", "A4", "A4", "G4", "F4", "F4", "E4", "E4", "D4", "D4", "C4", "G4", "G4", "F4", "F4", "E4", "E4", "D4", "G4", "G4", "F4", "F4", "E4", "E4", "D4", "C4", "C4", "G4", "G4", "A4", "A4", "G4", "F4", "F4", "E4", "E4", "D4", "D4", "C4"};
    // double interval_ms[] = {250.0, 250.0, 250.0, 250.0, 250.0, 250.0, 500.0, 250.0, 250.0, 250.0, 250.0, 250.0, 250.0, 500.0, 250.0, 250.0, 250.0, 250.0, 250.0, 250.0, 500.0, 250.0, 250.0, 250.0, 250.0, 250.0, 250.0, 500.0, 250.0, 250.0, 250.0, 250.0, 250.0, 250.0, 500.0, 250.0, 250.0, 250.0, 250.0, 250.0, 250.0, 500.0};
    // int size_notes = 42;
    std::string notes[] = {"A4", "B4", "C5"};
    double interval_ms[] = {400.0, 400.0, 400.0};
    int size_notes = 3;

    float *song = new float[size_notes];

    make_song_array(song, notes, size_notes);

    float decay = 1;


    // Enable speaker
    sd_pin_val = static_cast<enum gpiod_line_value>(1);
    gpiod_line_request_set_value(gpio_request, gpio_offsets[6], sd_pin_val);

    while (true) {
        
        t1t = high_resolution_clock::now();
        // Obtain target frequency and amplitude from 
        current_wave = f1.wave;
        if (current_wave == FREEBIRD) {
            if (start) {
                k = 0;
                t = 0;
                t_last = 0;
                start = false;
            }
            // current_frequency = song[k];
            current_frequency = f1.f;
            led_frequency = song[k];
        }
        else {
            current_frequency = f1.f;
            led_frequency = current_frequency;
            decay = 1;
            start = true;
        }
        current_amplitude = f1.ampl;
        visualisation_select = f1.vis_sett;
        // printf("Frequency: %.2f Hz, Amplitude: %.2f, Waveform: %d\n", current_frequency, current_amplitude, current_wave);
        t2t = high_resolution_clock::now();
        ms_double = t2t - t1t;
        // printf("Parameter aquisition: %f\n", ms_double.count());

        t1t = high_resolution_clock::now();

        // printf("current frequency: %.2f Hz\n", current_frequency);
        // Fill the current block buffer
        for (int i = 0; i < BUFFER_SIZE; i++) {
            if (current_wave == FREEBIRD) {
                decay = pow(M_E, -4 * (t-t_last));
            }
            // Calculate the wave value using the current accumulated phase
            switch (current_wave) {
                case SINE:
                    buffer[i] = (short)(sin(phase) * 32767.0 * current_amplitude * decay);
                    break;
                case BLOCK:
                    if (phase <= M_PI){
                        buffer[i] = (short) 32767.0 * current_amplitude * decay;
                    }
                    else{
                        buffer[i] = (short) -32767.0 * current_amplitude * decay;
                    }
                    break;
                case TRIANGLE:
                    if (phase <= M_PI){
                        buffer[i] = (short)((2*phase*32767.0/M_PI-32767.0) * current_amplitude * decay);
                    }
                    else{
                        buffer[i] = (short)((2*(M_PI-phase)*32767.0/M_PI+32767.0) * current_amplitude * decay);
                    }
                    break;
                case FREEBIRD:
                    if (phase <= M_PI){
                        buffer[i] = (short)((2*phase*32767.0/M_PI-32767.0) * current_amplitude * decay);
                    }
                    else{
                        buffer[i] = (short)((2*(M_PI-phase)*32767.0/M_PI+32767.0) * current_amplitude * decay);
                    }
                    break;
                default:
                    break;
            }

            // Advance the phase smoothly based ONLY on the immediate target frequency
            double phase_step = (2.0 * M_PI * current_frequency) / SAMPLE_RATE;
            phase += phase_step;

            // Prevent the phase float from growing infinitely and losing mathematical precision
            if (phase >= 2.0 * M_PI) {
                phase -= 2.0 * M_PI;
            }
            if (current_wave == FREEBIRD) {
                t += 1.0 / 48000.0;
                if (t - t_last >= interval_ms[k] / 1000.0) {
                    k += 1;
                    t_last = t;
                    current_frequency = song[k];
                    led_frequency = song[k];
                }
                if (k == size_notes) {
                    k = 0;
                    t = 0;
                    t_last = 0;
                }
            }
        }
        t2t = high_resolution_clock::now();
        ms_double = t2t - t1t;
        // printf("Wave generation: %f\n", ms_double.count());

        t1t = high_resolution_clock::now();

        // Send frames to ALSA pipeline
        snd_pcm_sframes_t frames = snd_pcm_writei(handle, buffer, BUFFER_SIZE);
        if (frames < 0) {
            frames = snd_pcm_recover(handle, frames, 0);
        }
        if (frames < 0) {
            fprintf(stderr, "snd_pcm_writei failed: %s\n", snd_strerror(frames));
            cleanup_gpio();
            break;
        }

        t2t = high_resolution_clock::now();
        ms_double = t2t - t1t;
        // printf("Send audio buffer: %f\n", ms_double.count());

        t1t = high_resolution_clock::now();

        buffer[0] = visualisation_select;

        sendto( sock,
                buffer,
                sizeof(buffer),
                0,
                (sockaddr*)&addr,
                sizeof(addr));

        t2t = high_resolution_clock::now();
        ms_double = t2t - t1t;
        // printf("Send to visualisation: %f\n\n", ms_double.count());
    }
    
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    cleanup_gpio();
    printf("Done!\n");
    return 0;
}