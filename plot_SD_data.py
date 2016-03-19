"""
Mutatio SD-daten Auswertung by Damian Schneider
"""

import matplotlib.pyplot as plt
import matplotlib.dates as md
import numpy as np
import datetime as dt
import time
import matplotlib.patches as patches
import matplotlib.path as path


#simply plot a file, no adjustments:

#plt.plotfile('/Users/daedae/PycharmProjects/test/netzsinus/2016067.txt', delimiter='\t', cols=(0, 1),
#             names=('Frequenz', 'Zeit'), marker='o')
#plt.show()


#plot a file with possibility to reformat the data

#enter path to your file here
with open('/Users/daedae/PycharmProjects/test/netzsinus/2016059_eu.txt') as f:
    data = f.read().splitlines()

timestamps = [row.split('\t')[0] for row in data]
timestamps = np.array(timestamps).astype(np.float) #make floats from strings
frequency = [row.split('\t')[1] for row in data]
frequency = np.array(frequency).astype(np.float) #make floats from strings
quality = [row.split('\t')[2] for row in data]
quality = np.array(quality).astype(np.int) #make integers from strings


datetimestamps = [dt.datetime.fromtimestamp(idx) for idx in timestamps]

#Mutatio code bug (is fixed now): overflows erkennen und bereinigen
for i in range(1, len(frequency)):
    if(frequency[i]>50.2 and frequency[i-1] < 49.8): #underflow value
      while(frequency[i] > 50.0):
        frequency[i] = frequency[i]-0.65536
        i += 1
    if(frequency[i]<49.8 and frequency[i-1] > 50.2): #overflow value
      while(frequency[i] < 50.0):
        frequency[i] = frequency[i]+0.65536
        i += 1


fig = plt.figure(1)

ax = fig.add_subplot(111)

ax.set_title("Grid Frequency")
#ax1.set_xlabel('Zeit')
ax.set_ylabel('Frequency')

plt.subplots_adjust(bottom=0.3)
plt.xticks( rotation=25 )

ax=plt.gca()
xfmt = md.DateFormatter('%Y-%m-%d %H:%M:%S')
ax.xaxis.set_major_formatter(xfmt)

ax.plot(datetimestamps,frequency, c='b', label='Frequency')
#leg = ax1.legend()  #show the legend label

plt.show()



#histogramm und fourier analyse:




fig = plt.figure(2)
plt.subplots_adjust(hspace=0.4)

ax = fig.add_subplot(211) #plt.subplots()
# histogram our data with numpy
n, bins = np.histogram(frequency, 1000)

# get the corners of the rectangles for the histogram
left = np.array(bins[:-1])
right = np.array(bins[1:])
bottom = np.zeros(len(left))
top = bottom + n

ax.axes.get_yaxis().set_visible(False) #disable drawing the y axis

# we need a (numrects x numsides x 2) numpy array for the path helper
# function to build a compound path
XY = np.array([[left, left, right, right], [bottom, top, top, bottom]]).T

# get the Path object
barpath = path.Path.make_compound_path_from_polys(XY)

# make a patch out of it
patch = patches.PathPatch(
    barpath, facecolor='gold', edgecolor='gold', alpha=0.8)
ax.add_patch(patch)

# update the view limits
ax.set_xlim(left[0], right[-1])
ax.set_ylim(bottom.min(), top.max()+top.max()*0.05)
ax.set_title("Frequency Distribution")
#plt.show()

#fourier analysis
#the problem is uneven spaced time data, lets make it one value per second data using cubic interpolation

from scipy.interpolate import interp1d
from scipy.fftpack import fft

frequency_interp = interp1d(timestamps, frequency) #, kind='cubic'
timestamps_interp = np.linspace(timestamps[0], timestamps[-1], int(timestamps[-1] - timestamps[0]), endpoint=True)

#now we have interpolated data, do the FFT

# Number of samplepoints
N = int(timestamps[-1] - timestamps[0])
# sample spacing
T = 1.0 #1Hz or 1 sample per second
x = np.linspace(0.0, N*T, N)
yf = fft(frequency_interp(timestamps_interp))
xf = np.linspace(0.0, T,N,endpoint=True)
#xf = np.linspace(timestamps[0], timestamps[-1], int(timestamps[-1] - timestamps[0]), endpoint=True)



ax = fig.add_subplot(212)
ax.set_title("Fourier analysis of Frequency")
ax.set_xlabel('Oscillations in the Frequency in [Hz]')

#ax.set_yscale('log')
ax.axes.get_yaxis().set_visible(False) #disable drawing the y axis
ax.axes.get_yaxis().set_visible(True) #disable drawing the y axis


plt.plot(xf[N/15:N/2],np.abs(yf[N/15:N/2]), c='r')
plt.grid()
plt.show()
