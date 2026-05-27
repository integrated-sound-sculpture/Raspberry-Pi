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

#include <unistd.h>
#include <thread>
#include <chrono>

#include <fcntl.h>			//Used for UART
#include <termios.h>		//Used for UART

#include <wiringSerial.h>

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

typedef enum{
    SINE,
    BLOCK,
    TRIANGLE
}FORM;

typedef struct freq{
    float f;
    FORM wave;
    float ampl;
}freq;

#define PORT        8080 
#define MAXLINE     1024 

#define WAVE_FORM SINE
#define SAMPLE_RATE 44100
#define TOTAL_FRAMES (SAMPLE_RATE * 4) // Play for 5 seconds total
#define BUFFER_SIZE 512                // Smaller buffers provide faster frequency updates

short buffer[BUFFER_SIZE];

freq f1{
    220.0f,
    SINE,
    1.0f
};

int sock = socket(AF_INET, SOCK_DGRAM, 0);

//-------------------------
	//----- SETUP USART 0 -----
	//-------------------------
	//At bootup, pins 8 and 10 are already set to UART0_TXD, UART0_RXD (ie the alt0 function) respectively
	int uart0_filestream = -1;
	
	//OPEN THE UART
	//The flags (defined in fcntl.h):
	//	Access modes (use 1 of these):
	//		O_RDONLY - Open for reading only.
	//		O_RDWR - Open for reading and writing.
	//		O_WRONLY - Open for writing only.
	//
	//	O_NDELAY / O_NONBLOCK (same function) - Enables nonblocking mode. When set read requests on the file can return immediately with a failure status
	//											if there is no input immediately available (instead of blocking). Likewise, write requests can also return
	//											immediately with a failure status if the output can't be written immediately.
	//
	//	O_NOCTTY - When set and path identifies a terminal device, open() shall not cause the terminal device to become the controlling terminal for the process.
	uart0_filestream = open("/dev/serial0", O_RDWR | O_NOCTTY | O_NDELAY);		//Open in non blocking read/write mode
	if (uart0_filestream == -1)
	{
		//ERROR - CAN'T OPEN SERIAL PORT
		printf("Error - Unable to open UART.  Ensure it is not in use by another application\n");
	}
	
	//CONFIGURE THE UART
	//The flags (defined in /usr/include/termios.h - see http://pubs.opengroup.org/onlinepubs/007908799/xsh/termios.h.html):
	//	Baud rate:- B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800, B500000, B576000, B921600, B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000
	//	CSIZE:- CS5, CS6, CS7, CS8
	//	CLOCAL - Ignore modem status lines
	//	CREAD - Enable receiver
	//	IGNPAR = Ignore characters with parity errors
	//	ICRNL - Map CR to NL on input (Use for ASCII comms where you want to auto correct end of line characters - don't use for bianry comms!)
	//	PARENB - Parity enable
	//	PARODD - Odd parity (else even)
	struct termios options;
	tcgetattr(uart0_filestream, &options);

void set_interface_attribs(int speed, int parity)
{
	options.c_cflag = B115200 | CS8 | CLOCAL | CREAD;		//<Set baud rate
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	tcflush(uart0_filestream, TCIFLUSH);
	tcsetattr(uart0_filestream, TCSANOW, &options);
}

void parameter_aquisition(void)
{
    while(true){
        //----- CHECK FOR ANY RX BYTES -----
        if (uart0_filestream != -1)
        {
            // Read up to 255 characters from the port if they are there
            unsigned char rx_buffer[256];
            int rx_length = read(uart0_filestream, (void*)rx_buffer, 255);		//Filestream, buffer to store in, number of bytes to read (max)
            if (rx_length < 0)
            {
                //An error occured (will occur if there are no bytes)
            }
            else if (rx_length == 0)
            {
                //No data waiting
            }
            else
            {
                //Bytes received
                rx_buffer[rx_length] = '\0';
                printf("%i bytes read : %s\n", rx_length, rx_buffer);
            }
        }
        f1.f += 10.0;
        if(f1.f >= 2000.0){
            f1.f = 220.0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

int main(void)
{
    int err;
    snd_pcm_t *handle;
    
    std::thread a_thread(parameter_aquisition);
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Open and configure the playback device
    err = snd_pcm_open(&handle, "hw:1", SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        fprintf(stderr, "Playback open error: %s\n", snd_strerror(err));
        return EXIT_FAILURE;
    }

    err = snd_pcm_set_params(handle,
                             SND_PCM_FORMAT_S16_LE,
                             SND_PCM_ACCESS_RW_INTERLEAVED,
                             1, SAMPLE_RATE, 1, 40000);
    if (err < 0) {
        fprintf(stderr, "Hardware configuration error: %s\n", snd_strerror(err));
        snd_pcm_close(handle);
        return EXIT_FAILURE;
    }

    printf("Sweeping frequency smoothly from 220Hz to 880Hz...\n");

    int frames_played = 0;
    double phase = 0.0; // The critical running phase accumulator

    //while (frames_played < TOTAL_FRAMES) {
    while (true) {
        
        // Calculate a target frequency that slides upward over time
        double current_frequency = f1.f; // Ramps from 220 to 880Hz

        // Fill the current block buffer
        for (int i = 0; i < BUFFER_SIZE; i++) {
            // Calculate the wave value using the current accumulated phase
            switch (WAVE_FORM) {
                case SINE:
                    buffer[i] = (short)(sin(phase) * 25000.0);
                    break;
                case BLOCK:
                    if (phase <= M_PI){
                        buffer[i] = (short) 30000.0;
                    }
                    else{
                        buffer[i] = (short) -30000.0;
                    }
                    break;
                case TRIANGLE:
                    if (phase <= M_PI){
                        buffer[i] = (short)(phase*60000.0/M_PI-30000.0);
                    }
                    else{
                        buffer[i] = (short)((M_PI-phase)*60000.0/M_PI+30000.0);
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
        }

        // Send frames to ALSA pipeline
        snd_pcm_sframes_t frames = snd_pcm_writei(handle, buffer, BUFFER_SIZE);
        if (frames < 0) {
            frames = snd_pcm_recover(handle, frames, 0);
        }
        if (frames < 0) {
            fprintf(stderr, "snd_pcm_writei failed: %s\n", snd_strerror(frames));
            break;
        }

        frames_played += BUFFER_SIZE;
        if (frames_played >= TOTAL_FRAMES) {
            frames_played = 0;
        }
        sendto( sock,
                buffer,
                sizeof(buffer),
                0,
                (sockaddr*)&addr,
                sizeof(addr));
    }
    
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    printf("Done!\n");
    return 0;
}

/*
#include <iostream>
#include <cmath>
#include <alsa/asoundlib.h>

#include <stdio.h>
#include <stdlib.h>

// Allocate an array filled with arbitrary audio bytes
unsigned char buffer[16 * 1024]; 

int main(void) {
    int err;
    snd_pcm_t *handle;

    // Fill buffer with random static noise
    for (size_t i = 0; i < sizeof(buffer); i++) {
        buffer[i] = rand() & 0xff;
    }
    

    // 1. Open default sound card for playback pipeline
    err = snd_pcm_open(&handle, "hw:1", SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        fprintf(stderr, "Playback open error: %s\n", snd_strerror(err));
        return EXIT_FAILURE;
    }

    // 2. Set structural hardware configs: Unsigned 8-bit, Mono, 44.1kHz, 0.5s latency tolerance
    err = snd_pcm_set_params(handle,
                             SND_PCM_FORMAT_U8,
                             SND_PCM_ACCESS_RW_INTERLEAVED,
                             1,      // 1 Channel (Mono)
                             44100,  // Sample Rate
                             1,      // Allow ALSA software resampling
                             500000); // 500,000 microseconds latency
    if (err < 0) {
        fprintf(stderr, "Hardware configuration error: %s\n", snd_strerror(err));
        snd_pcm_close(handle);
        return EXIT_FAILURE;
    }

    // 3. Send audio blocks to your hardware output frame loop
    printf("Playing static burst...\n");
    for (int i = 0; i < 16; i++) {
        snd_pcm_sframes_t frames = snd_pcm_writei(handle, buffer, sizeof(buffer));
        if (frames < 0) {
            frames = snd_pcm_recover(handle, frames, 0); // Handle buffer underruns automatically
        }
        if (frames < 0) {
            fprintf(stderr, "snd_pcm_writei failed entirely: %s\n", snd_strerror(frames));
            break;
        }
    }

    // 4. Safely flush residual buffer signals out and lock down audio channel
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    printf("Done!\n");
    return 0;
}



int main() {
    // 1. Open PCM device for playback
    snd_pcm_t *pcm_handle;
    int rc = snd_pcm_open(&pcm_handle, "hw:1", SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0) {
        std::cerr << "Error opening PCM device: " << snd_strerror(rc) << std::endl;
        return -1;
    }

    // 2. Set hardware parameters
    snd_pcm_hw_params_t *params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(pcm_handle, params);
    
    // Set 16-bit little-endian format
    snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE);
    
    unsigned int sample_rate = 44100;
    snd_pcm_hw_params_set_channels(pcm_handle, params, 2); // Stereo
    snd_pcm_hw_params_set_rate_near(pcm_handle, params, &sample_rate, 0);

    // Apply parameters
    rc = snd_pcm_hw_params(pcm_handle, params);
    if (rc < 0) {
        std::cerr << "Error setting HW parameters: " << snd_strerror(rc) << std::endl;
        snd_pcm_close(pcm_handle);
        return -1;
    }

    // 3. Prepare buffer (Generate a 440Hz sine wave tone)
    int buffer_frames = 128;
    short *buffer = new short[buffer_frames * 2]; // 2 channels
    double frequency = 440.0;
    double phase = 0.0;

    // Play for 3 seconds
    for (int i = 0; i < 3 * 44100 / buffer_frames; ++i) {
        for (int frame = 0; frame < buffer_frames; ++frame) {
            // Generate sine value (-32767 to 32767)
            short val = (short)(sin(phase) * 32767.0);
            phase += (2.0 * M_PI * frequency) / sample_rate;
            if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;

            // Interleaved: Left channel, Right channel
            buffer[frame * 2] = val;
            buffer[frame * 2 + 1] = val;
        }

        // Write data to ALSA
        rc = snd_pcm_writei(pcm_handle, buffer, buffer_frames);
        if (rc == -EPIPE) {
            std::cerr << "Underrun occurred!" << std::endl;
            snd_pcm_prepare(pcm_handle);
        } else if (rc < 0) {
            std::cerr << "Error writing to PCM: " << snd_strerror(rc) << std::endl;
        }
    }

    // 4. Cleanup
    delete[] buffer;
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
    return 0;
}*/
