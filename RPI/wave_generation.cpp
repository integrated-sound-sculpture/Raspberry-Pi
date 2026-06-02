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

#include <wiringSerial.h>

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

typedef enum{
    SINE,
    BLOCK,
    TRIANGLE
}FORM;

typedef struct settings{
    // std::atomic<float> f;
    // std::atomic<FORM> wave;
    // std::atomic<float> ampl;
    float f;
    FORM wave;
    float ampl;
}settings;

#define PORT        8080 
#define MAXLINE     1024 

#define WAVE_FORM SINE
#define SAMPLE_RATE 44100
#define TOTAL_FRAMES (SAMPLE_RATE * 4) // Play for 5 seconds total
#define BUFFER_SIZE 512                // Smaller buffers provide faster frequency updates

short buffer[BUFFER_SIZE];

settings f1{
    440.0f,
    WAVE_FORM,
    1.0f
};

int sock = socket(AF_INET, SOCK_DGRAM, 0);

//-------------------------
//----- SETUP USART 0 -----
//-------------------------
//At bootup, pins 8 and 10 are already set to UART0_TXD, UART0_RXD (ie the alt0 function) respectively
int uart0_filestream = -1;

struct termios options;


void set_interface_attribs(void)
{
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
    uart0_filestream = open("/dev/ttyS0", O_RDWR | O_NOCTTY);		//Open in non blocking read/write mode
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
    /*
    tcgetattr(uart0_filestream, &options);
	options.c_cflag = B115200 | CS8 | CLOCAL | CREAD;		//<Set baud rate
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	tcflush(uart0_filestream, TCIFLUSH);
	tcsetattr(uart0_filestream, TCSANOW, &options);
    */
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
    options.c_iflag = 0;

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
        else
        {
            // optional timeout/failure handling
            usleep(1000);
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
            // Read up to 255 characters from the port if they are there
            // unsigned char rx_buffer[256];
            
            char line[128];

            if(read_line(uart0_filestream, line, sizeof(line)))
            {
                float freq;
                int ampl;
                
                printf("RX = [%s]\n", line);
                printf("%d\n", sscanf(line, "%f,%d", &freq, &ampl) == 2);

                if(sscanf(line, "%f,%d", &freq, &ampl) == 2)
                {
                    printf("Hello");
                    //f1.f = freq;
                }
            }
/*
            int rx_length = read(uart0_filestream, rx_buffer, sizeof(rx_buffer)-1);

            if(rx_length > 0)
            {
                rx_buffer[rx_length] = '\0';
            }
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
*/
        }
        //std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

int main(void)
{
    set_interface_attribs();
    
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
            switch (f1.wave) {
                case SINE:
                    buffer[i] = (short)(sin(phase) * 30000.0);
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