#import pyqtgraph.examples
import ctypes
#ctypes.windll.ole32.OleInitialize(None)

from scipy.io import wavfile
import numpy as np
import pyqtgraph as pg
import sys
import sounddevice as sd
import threading
from scipy.fft import fft, fftfreq
from scipy.signal import spectrogram
import socket
from pyqtgraph.Qt import QtCore, QtWidgets, QtGui


# sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
# sock.bind(("127.0.0.1", 8080))

# def main():
#     while True:
#         data, addr = sock.recvfrom(2048)
#         samples = np.frombuffer(data, dtype=np.int16)
#         sample_rate = 44100
#         print(samples)
#     return 0

# if _name_ == '_main_':
#     import sys
#     main()


app = QtWidgets.QApplication(sys.argv)                              #start the application

#pyqtgraph.examples.run()
sample_rate, samples = wavfile.read(r"C:\BAP\liecio-spook-theremin-257588.wav")        #read out the wav file

if samples.ndim > 1:                                                #in the case of stereo, take one channel
    samples = samples[:, 0]

samples = samples.astype(np.float32)                                #convert to float32 for processing, int will cut off values too much
samples = samples / np.max(np.abs(samples))                         #normalize samples to -1.0 to 1.0 

chunk_size = 1024                                               
time = np.arange(chunk_size) / sample_rate                          #time array for one chunk, used for plotting the waveform
chunk = samples[:chunk_size]                                        #audio data chunk
current_chunk = chunk.copy()                                        #copy of the current chunk, used as a register

fft_values = np.abs(np.array(fft(chunk)))[:chunk_size//2]           #FFT values for the chunk // 2 to only take positive frequencies
fft_freqs = fftfreq(chunk_size, 1/sample_rate)[:chunk_size//2]      #frequency corresponding to fft_values

win = QtWidgets.QWidget()                                           #window setup
win.setWindowTitle('Audio Visualization')
layout = QtWidgets.QVBoxLayout()
win.setLayout(layout)

selector = QtWidgets.QComboBox()                                    #create dropdown selector
selector.addItems(['Sine Wave', 'FFT', 'Spectrogram'])
layout.addWidget(selector)

p1_ymax = 0.1                                                       #initial y-axis max for waveform, will be adjusted dynamically
p2_ymax = 0.1   

freq_labels = [20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000]    

p1 = pg.PlotWidget(title= 'sine')                                   #plot waveform
p1.setLabel('bottom', 'Time', units='s')
p1.setLabel('left', 'Amplitude')
graph1 = p1.plot(time.flatten(), chunk.astype(float).flatten())     #flatten to use with pyqtgraph
layout.addWidget(p1)

p2 = pg.PlotWidget(title= 'FFT')                                    #plot FFT     
p2.setLabel('bottom', 'Frequency', units='Hz')
p2.setLabel('left', 'Magnitude')
graph2 = p2.plot(fft_freqs.flatten(), fft_values.flatten())
tick_positions = [(np.log10(f), f'{f} Hz') for f in freq_labels]  #afstand van fft_freqs tot de gewenste frequenties, en labels maken
p2.getAxis('bottom').setTicks([tick_positions])                       #set frequency labels on the x-axis
p2.setXRange(0, sample_rate//2)                                              #limit x-axis to Nyquist frequency
p2.setLogMode(x=True, y=False)                                                              #set logarithmic x-axis
p2.hide()
layout.addWidget(p2)

p3 = pg.PlotWidget(title= 'Spectrogram')                            #plot spectrogram
p3.setLabel('bottom', 'Time (frames)')
p3.setLabel('left', 'Frequency')
graph3 = pg.ImageItem()
p3.addItem(graph3)

graph3.setColorMap('viridis')

f_test, t_test, Sxx_test = spectrogram(chunk, fs=sample_rate, nperseg=256, noverlap=128)   
n_freqs = len(f_test)                                               
waterfall_buffer = np.zeros((n_freqs, 50)) - 80                     # buffer maken voor de waterfall plot, -80 om de eerste frames donker te maken
log_freqs = np.logspace(np.log10(20), np.log10(sample_rate//2), n_freqs)

freq_labels = [20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000]       #poging tot logaritmisch laten lijken
tick_positions = [(int(np.argmin(np.abs(log_freqs - f))), f'{f} Hz') for f in freq_labels]  # afstand van log_freqs tot de gewenste frequenties, en labels maken
p3.getAxis('left').setTicks([tick_positions])                       #set log frequency labels on the x-axis
p3.hide()
layout.addWidget(p3)

def on_select(index):                                               #function to switch between plots
    if index == 0:
        p1.show()
        p2.hide()
        p3.hide()
    elif index == 1:
        p1.hide()
        p2.show()
        p3.hide()
    else:
        p1.hide()
        p2.hide()
        p3.show()

selector.currentIndexChanged.connect(on_select)

audio_running = True
def audio_thread():                                                #Test thread fit audio to the plots
    global current_chunk
    audio_pos = 0
    with sd.OutputStream(samplerate=sample_rate, channels=1) as stream:
        while audio_running:
            if audio_pos + chunk_size > len(samples):
                audio_pos = 0
            chunk = samples[audio_pos:audio_pos + chunk_size]
            current_chunk = chunk.copy()
            if not audio_running:
                break
            stream.write(chunk)        
            audio_pos += chunk_size

thread = threading.Thread(target=audio_thread, daemon=True)
thread.start()

def on_app_quit():                                                  #fix the error of closing while thread is still running
    global audio_running
    audio_running = False
    thread.join(timeout=1.0)

app.aboutToQuit.connect(on_app_quit)                                # Connect the cleanup function to the application's quit signal

def update():                                                       #update function for the plots, called by a timer
    global waterfall_buffer, current_chunk, p1_ymax, p2_ymax
    chunk = current_chunk.copy()                                    #making use of the register

    fft_values = np.abs(np.array(fft(chunk)))[:chunk_size//2]       #perform fft, put it in an array and take the absolute value, for only positive frequencies
    fft_freqs = fftfreq(chunk_size, 1/sample_rate)[:chunk_size//2]  #get corresponding frequencies for the fft values

    f,t,Sxx = spectrogram(chunk, fs=sample_rate, nperseg=256, noverlap=128) #make spectrogram
    magnitude_db_spec = 10 * np.log10(Sxx + 1e-10)                  #convert magnitude to logaritmic, avoid log(0)
    col = magnitude_db_spec.mean(axis=1)                            #average over time to get a single column for the waterfall plot    
    col_log = np.interp(log_freqs, f, col)                          #interpolate to match the log frequency bins

    graph1.setData(time.flatten(), chunk.astype(float).flatten())   #update waveform plot
    graph2.setData(fft_freqs.flatten(), fft_values.flatten())       #update FFT plot
    waterfall_buffer = np.roll(waterfall_buffer, -1, axis=1)        #shift buffer to the left
    waterfall_buffer[:, -1] = col_log                               #add new column to the right of the buffer  
    graph3.setImage(waterfall_buffer.T, autoLevels=True)

    if np.max(np.abs(chunk)) > p1_ymax:                             #dynamically adjust y-axis if the signal exceeds 90% of the current max
        p1_ymax = p1_ymax * 1.5
        p1.setYRange(-p1_ymax, p1_ymax)
    if np.max(fft_values) > p2_ymax:
        p2_ymax = p2_ymax * 1.5
        p2.setYRange(0, p2_ymax)


timer = QtCore.QTimer()                                             #timer to call the update function every 50 ms
timer.timeout.connect(update)
timer.start(50)

win.resize(800, 600)
win.show()
app.exec()