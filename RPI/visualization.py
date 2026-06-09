#import pyqtgraph.examples
import ctypes
#ctypes.windll.ole32.OleInitialize(None)

from scipy.io import wavfile
import numpy as np
import pyqtgraph as pg
import sys
# import sounddevice as sd
import threading
from scipy.fft import fft, fftfreq
from scipy.signal import spectrogram
# import soundfile as sf
import socket
from pyqtgraph.Qt import QtCore, QtWidgets, QtGui


sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(("127.0.0.1", 8080))

sample_rate = 44100

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
# samples, sample_rate = sf.read(r"C:\BAP\Theremin_sci-fi.wav", dtype='float32')        #read out the wav file

# if samples.ndim > 1:                                                #in the case of stereo, take one channel
#     samples = samples[:, 0]

# samples = samples.astype(np.float32)                                #convert to float32 for processing, int will cut off values too much
# samples = samples / np.max(np.abs(samples))                         #normalize samples to -1.0 to 1.0 

chunk_size = 512                                               
time = np.arange(chunk_size) / sample_rate                          #time array for one chunk, used for plotting the waveform
chunk = np.zeros(chunk_size)                                   #audio data chunk
current_chunk = chunk.copy()                                        #copy of the current chunk, used as a register

fft_values = np.abs(np.array(fft(chunk)))[:chunk_size//2]           #FFT values for the chunk // 2 to only take positive frequencies
fft_freqs = fftfreq(chunk_size, 1/sample_rate)[:chunk_size//2]      #frequency corresponding to fft_values

win = QtWidgets.QWidget()                                           #window setup
win.setWindowTitle('Audio Visualization')
layout = QtWidgets.QVBoxLayout()
win.setLayout(layout)

selector = QtWidgets.QComboBox()                                    #create dropdown selector
selector.addItems(['All', 'Sine Wave', 'Spectrogram','Notes'])
layout.addWidget(selector)

p1_ymax = 0.1                                                       #initial y-axis max's, will be adjucted in update()
p2_ymax = 0.1 
p3_fft_ymax = 0.1  

f_test, t_test, Sxx_test = spectrogram(chunk, fs=sample_rate, nperseg=256, noverlap=128)   #only f-test is used, nperseg is window size, noverlap is amount op points that overlap between windows
n_freqs = len(f_test)                                               # 256 / 2 + 1 = 129, single sided, including the zero
waterfall_buffer = np.zeros((n_freqs, 50)) - 80                     # buffer maken voor de waterfall plot, -80 om de eerste frames donker te maken
log_freqs = np.logspace(np.log10(20), np.log10(sample_rate//2), n_freqs) #log frequencies van 20 tot nyquist

freq_labels = [20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000]
tick_positions = [(np.log10(f), f'{f} Hz') for f in freq_labels]              #afstand van fft_freqs tot de frequenties
tick_positions_log = [(int(np.argmin(np.abs(log_freqs - f))), f'{f}') for f in freq_labels]  #afstand van fft_freqs tot de frequenties

note_labels = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'] #Note labels, for some reason it starts with C
midi_min = 21 
midi_max = 108
midi_freqs = 440 * 2 ** ((np.arange(midi_min, midi_max + 1) - 69) / 12)  #freq die bij elke midi toon hoort

amplitude_threshold = 2  #drempel voor het tonen van de gedetecteerde noot, om ruis te voorkomen

p1 = pg.PlotWidget(title= 'sine')                                   #plot waveform
p1.setLabel('bottom', 'Time', units='s')
p1.setLabel('left', 'Amplitude')
graph1 = p1.plot(time.flatten(), chunk.astype(float).flatten())     #flatten to use with pyqtgraph
layout.addWidget(p1)

p2 = pg.PlotWidget(title= 'FFT')                                    #plot FFT     
p2.setLabel('bottom', 'Frequency', units='Hz')
p2.setLabel('left', 'Magnitude')
graph2 = p2.plot(fft_freqs.flatten(), fft_values.flatten())
p2.setXRange(20, sample_rate//2)                                              #limit x-axis to Nyquist frequency
p2.setLogMode(x=True, y=False)                                                              #set logarithmic x-axis
tick_positions = [(np.log10(f), f'{f} Hz') for f in freq_labels]  #afstand van fft_freqs tot de frequenties
p2.hide()
layout.addWidget(p2)

p3_packet = QtWidgets.QWidget()                                         #maak een pakket van zowel fft als spetrogram
p3_layout = QtWidgets.QVBoxLayout()                                     #layout voor plots onder elkaar
p3_packet.setLayout(p3_layout)                                          #put the layout in the packet

p3_fft = pg.PlotWidget(title= 'FFT')                                    #plot FFT of the packet
p3_fft.setLabel('left', 'Magnitude')
p3_fft.getAxis('left').setWidth(60)                                     #a constant width for y axis to ensure both y axis are the same
#p3_fft.setLabel('bottom', 'Frequency', units='Hz')
graph3_fft = p3_fft.plot(fft_freqs.flatten(), fft_values.flatten())     #flatten to use with pyqtgraph
#p3_fft.getAxis('bottom').setTicks([tick_positions])                       #set frequency labels on the x-axis
#p3_fft.setXRange(20, sample_rate//2)                                              #limit x-axis
p3_fft.getAxis('bottom').setStyle(showValues=False)                         #hide x-axis labels for the fft plot in the packet becasue they are shown in the spectrogram
p3_fft.setYRange(0, p3_fft_ymax)                                              #initial y-axis range
p3_layout.addWidget(p3_fft)

p3 = pg.PlotWidget()#title= 'Spectrogram')                            #plot spectrogram
p3.setLabel('left', 'Time (frames)')
p3.setLabel('bottom', 'Frequency', units='Hz')
p3.getAxis('left').setWidth(60)
graph3 = pg.ImageItem()
p3.addItem(graph3)
graph3.setColorMap('inferno')                                                 #colormap for the spectrogram, other option is viridis

p3.getAxis('bottom').setTicks([tick_positions_log])                       #set log frequency labels on the x-axis
p3_layout.addWidget(p3)
layout.addWidget(p3_packet)

p3_fft.setXLink(p3)                                                     #link the axis together

p4 = pg.PlotWidget(title= 'Notes')                                  #plot note
p4.setAspectLocked(False)
p4.setYRange(0, 1)
p4.setXRange(0, 88)
p4.getAxis('bottom').setStyle(showValues=False)
p4.getAxis('left').setStyle(showValues=False)

white_keys = []
black_keys = []
white_key_items = {}
black_key_items = {}

white_pattern = [0,2,4,5,7,9,11]                                    # to connect midi notes to the right color key
black_pattern = [1,3,6,8,10]

white_x = 0
white_positions = {}
for midi in range(midi_min, midi_max + 1):                          #loop through midi notes and assign positions for white keys
    note = midi % 12
    if note in white_pattern:
        white_positions[midi] = white_x
        white_x += 1                            

for midi, x in white_positions.items():                             #create rectangles for white keys based
    rect = QtWidgets.QGraphicsRectItem(x, 0, 1, 1)
    rect.setBrush(pg.mkBrush('w')  )
    rect.setPen(pg.mkPen('k'))
    p4.addItem(rect)
    white_key_items[midi] = rect

for midi in range(midi_min, midi_max + 1):                          #loop through midi notes and create rectangles for black keys, offset calculated based on preious white key
    note = midi % 12
    if note in black_pattern:
        prev_white_x = midi - 1
        if prev_white_x in white_positions:
            x = white_positions[prev_white_x] + 0.6
            rect = QtWidgets.QGraphicsRectItem(x, 0.4, 0.6, 0.6)
            rect.setBrush(pg.mkBrush('k')  )
            rect.setPen(pg.mkPen('k'))
            p4.addItem(rect)
            black_key_items[midi] = rect
p4.setXRange(0, white_x)
layout.addWidget(p4)

def on_select(index):                                               #function to switch between plots
    if index == 0:
        p1.show()
        p2.hide()
        p3_packet.show()
        p4.show()
    elif index == 1:
        p1.show()
        p2.hide()
        p3_packet.hide()
        p4.hide()
    elif index == 2:
        p1.hide()
        p2.hide()
        p3_packet.show()
        p4.hide()
    else:
        p1.hide()
        p2.hide()
        p3_packet.hide()
        p4.show()

selector.currentIndexChanged.connect(on_select)

# audio_running = True
# def audio_thread():                                                #Test thread fit audio to the plots
#     global current_chunk
#     audio_pos = 0
#     with sd.OutputStream(samplerate=sample_rate, channels=1) as stream:
#         while audio_running:
#             if audio_pos + chunk_size > len(samples):
#                 audio_pos = 0
#             chunk = samples[audio_pos:audio_pos + chunk_size]
#             current_chunk = chunk.copy()
#             if not audio_running:
#                 break
#             stream.write(chunk)        
#             audio_pos += chunk_size

# thread = threading.Thread(target=audio_thread, daemon=True)
# thread.start()

def on_app_quit():                                                  #fix the error of closing while thread is still running
    # global audio_running
    # audio_running = False
    # thread.join(timeout=1.0)
    pass

app.aboutToQuit.connect(on_app_quit)                                # Connect the cleanup function to the application's quit signal

smooth_midi = [69.0]

def update():                                                       #update function for the plots, called by a timer
    global waterfall_buffer, current_chunk, p1_ymax, p2_ymax, p3_fft_ymax
    
    data, addr = sock.recvfrom(chunk_size * 2)
    chunk = np.frombuffer(data, dtype=np.int16)
    
    # chunk = current_chunk.copy()                                    #making use of the register

    fft_values = np.abs(np.array(fft(chunk)))[:chunk_size//2]       #perform fft, put it in an array and take the absolute value, for only positive frequencies
    fft_freqs = fftfreq(chunk_size, 1/sample_rate)[:chunk_size//2]  #get corresponding frequencies for the fft values
    fft_values_log = np.interp(log_freqs, fft_freqs, fft_values)    #interpolate the fft values to match the log frequency bins

    f,t,Sxx = spectrogram(chunk, fs=sample_rate, nperseg=256, noverlap=128) #make spectrogram
    magnitude_db_spec = 10 * np.log10(Sxx + 1e-10)                  #convert magnitude to logaritmic, avoid log(0)
    col = magnitude_db_spec.mean(axis=1)                            #average over time to get a single column for the waterfall plot    
    col_log = np.interp(log_freqs, f, col)                          #interpolate to match the log frequency bins

    graph1.setData(time.flatten(), chunk.astype(float).flatten())   #update waveform plot
    graph2.setData(fft_freqs.flatten(), fft_values.flatten())       #update FFT plot
    #graph3_fft.setData(log_freqs, fft_values_log)   #update FFT plot in the spectrogram packet
    graph3_fft.setData(np.arange(len(log_freqs)), fft_values_log)
    waterfall_buffer = np.roll(waterfall_buffer, -1, axis=1)        #shift buffer to the left
    waterfall_buffer[:, -1] = col_log                               #add new column to the right of the buffer  
    graph3.setImage(waterfall_buffer, autoLevels=True)

    if np.max(np.abs(chunk)) > p1_ymax:                             #dynamically adjust y-axis if the signal exceeds 90% of the current max
        p1_ymax = p1_ymax * 1.5
        p1.setYRange(-p1_ymax, p1_ymax)
    if np.max(fft_values) > p2_ymax:
        p2_ymax = p2_ymax * 1.5
        p2.setYRange(0, p2_ymax)
    if np.max(fft_values) > p3_fft_ymax:
        p3_fft_ymax = p3_fft_ymax * 1.5
        p3_fft.setYRange(0, p3_fft_ymax)

    valid_freq = fft_freqs > 20
    if np.any(valid_freq):
        peak_idx = np.argmax(fft_values[valid_freq])                #find the index of the peak in the FFT
        peak_freq = fft_freqs[valid_freq][peak_idx]                 #get the corresponding frequency
        peak_magnitude = fft_values[valid_freq][peak_idx]           #get the magnitude of the peak
        midi_note = 69 + 12 * np.log2(max(peak_freq / 440, 1e-10))  #convert frequency to MIDI note number

        center = round(midi_note)                                   #get the nearest MIDI note number to the smoothed value

        for midi, rect in white_key_items.items():                  #reset all colors
            rect.setBrush(pg.mkBrush('w'))
        for midi, rect in black_key_items.items():
            rect.setBrush(pg.mkBrush('k'))

        if peak_magnitude > amplitude_threshold:                    #only show the detected note if it exceeds the amplitude threshold
            if center in white_key_items:                           #highlight the note by making the key red
                white_key_items[center].setBrush(pg.mkBrush('r'))
            elif center in black_key_items:
                black_key_items[center].setBrush(pg.mkBrush('r'))
            note_name = note_labels[center % 12]                    #get the name of the detected note
            octave = center // 12 - 1
            p4.setTitle(f'Detected Note: {note_name}{octave} ({peak_freq:.1f} Hz) Amplitude: {peak_magnitude:.2f}')  #set the title to show the detected note and its frequency and amplitude

timer = QtCore.QTimer()                                             #timer to call the update function every 50 ms
timer.timeout.connect(update)
timer.start(50)

win.showMaximized()                                                 #show the window fullscreen
win.show()
app.exec()
