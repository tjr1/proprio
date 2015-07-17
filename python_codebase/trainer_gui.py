
import matplotlib as mpl
mpl.rcParams['toolbar'] = 'None'

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.widgets import Button
import serial
import datetime
import time
from types import *
import sys

# -------------------------------------------------------------------------
# GLOBAL DEFINITIONS 
MAX_SCOPE_TIME = 10


# GLOBAL VARIABLES
fig, ax = plt.subplots()
line, = ax.plot([], [], 'b*:', lw=2)
line_thresh, = ax.plot([], [], 'r:', lw=2)
ax.set_ylim(-1.1, 1.1)
ax.set_xlim(0, MAX_SCOPE_TIME)
ax.grid()
ax.margins(y=0.1)
#ax.autoscale_view(tight=None, scalex=False, scaley=True)
ax.set_ylim(auto=True)

xdata, ydata = [0], [0]

text_label1 = plt.text(0.9, 1.05, 'FPS:', \
						horizontalalignment='center', \
						verticalalignment='center', \
						transform=ax.transAxes)
text_FPS_val = plt.text(0.95, 1.05, '0', \
						horizontalalignment='center', \
						verticalalignment='center', \
						transform=ax.transAxes)

FLAG_PAUSE = False
time_start = datetime.datetime.now()

# Serial Port
_serialPort = serial.Serial()
_serialPort.baudrate = 115200
_serialPort.timeout = 0.001


# -------------------------------------------------------------------------
# FUNCTION DEFINITIONS
def pause_animation(event):
	global FLAG_PAUSE;
	FLAG_PAUSE = True

def play_animation(event):
	global FLAG_PAUSE;
	FLAG_PAUSE = False

def data_gen():
	t = data_gen.t
	cnt = 0
	while cnt < 1000:
		if not FLAG_PAUSE:
			cnt+=1
			t += 0.05
		yield t, np.sin(2*np.pi*t)		#* np.exp(-t/10.)
data_gen.t = 0

def data_gen_timeStamps():
	t = data_gen.t
	cnt = 0
	while cnt < 1000:
		if not FLAG_PAUSE:
			cnt+=1
			time_now = datetime.datetime.now()
			dt = time_now - time_start
			t = dt.seconds + dt.microseconds/1000000.;
		yield t, np.sin(2*np.pi*t)		#* np.exp(-t/10.)
data_gen.t = 0

def serialPortListener():
	rx_serial_data = serialPortListener.rx_serial_data
	cnt = 0
	profile_tstart = time.time()				# for profiling
	while True:
		if not FLAG_PAUSE:
			cnt += 1
			msg = _serialPort.readline()
			try:
				rx_serial_data = float(msg)
			except ValueError:
				rx_serial_data = -1

			time_now = datetime.datetime.now()
			dt = time_now - time_start
			t = dt.seconds + dt.microseconds/1000000.;

			if (cnt % 50 == 0):
				cnt = 0

				FPS = int(50/(time.time()-profile_tstart))
				text_FPS_val.set_text(FPS)
				#print 'FPS:' , FPS

				# for profiling
				profile_tstart = time.time()
				plt.draw()

		yield t, rx_serial_data
serialPortListener.rx_serial_data = 0

def run(data):
	# update the data
	t,y = data
	global xdata
	global ydata 

	if (y == -9999):
		exit(True)

	#if (y is not -1):
	xdata.append(t)
	ydata.append(y)

	xdata_plotted = [t-element for element in xdata]
	ydata_plotted = ydata
	if t >= MAX_SCOPE_TIME:
		size_xdata_plotted = len(xdata_plotted)
		xdata_plotted = [(element) for element in xdata_plotted \
							if element <= MAX_SCOPE_TIME]
		num_items_cut = size_xdata_plotted - len(xdata_plotted)
		ydata_plotted = ydata[num_items_cut:]

	line.set_data(xdata_plotted, ydata_plotted)


	sensor_data = [(element) for element in ydata_plotted if element >= 0]
	val_1 = np.mean(sensor_data)
	val_2 = np.sqrt(np.std(sensor_data))
	val_3 = val_1 + 3*val_2
	sensor_data = [(element) for element in sensor_data if element <= val_3]
	val_1 = np.mean(sensor_data)
	val_2 = np.sqrt(np.std(sensor_data))
	val_3 = val_1 + 100*val_2
	threshold = val_3

	line_thresh.set_data([min(xdata_plotted), max(xdata_plotted)], \
						[threshold,threshold])

	y_absmax = max(abs(element) for element in ydata_plotted)
	y_absmax += max([y_absmax*(1./10.), 1])
	ax.set_ylim([-1,y_absmax])

	return line, line_thresh

def exit(event):
	plt.close("all")


# -------------------------------------------------------------------------
# MAIN FUNCTION
def main(argv):

	COM_PORT_NUMBER = int(argv[0]) 
	_serialPort.port = COM_PORT_NUMBER-1
	_serialPort.timeout = 2
	_serialPort.open()	
	if (_serialPort.isOpen() == True):
		print "GUI: Opening COM port name: " + _serialPort.name
	else:
		print "GUI: Error!! Unable to open COM port: " + _serialPort.name

	# Wait for Initial Handshake from 'trainer_bot'
	_serialPort.write('Hello World!\n')
	rxdata = _serialPort.readline()
	if (rxdata == 'Device Status\n'):
		_serialPort.write('GUI Device 1\n')
	else:
		print 'GUI: Connection to \'trainer_bot\' failed!'
	_serialPort.timeout = 0

	# Initialize Animation Loop
	ani = animation.FuncAnimation(fig, run, serialPortListener, \
								blit=True, interval=10, \
								repeat=False)

	axprev = plt.axes([0.59, 0.05, 0.1, 0.075])
	axnext = plt.axes([0.7, 0.05, 0.1, 0.075])
	axexit = plt.axes([0.81, 0.05, 0.1, 0.075])

	bnext = Button(axnext, 'Pause')
	bnext.on_clicked(pause_animation)

	bprev = Button(axprev, 'Play')
	bprev.on_clicked(play_animation)
	
	bexit = Button(axexit, 'Exit')
	bexit.on_clicked(exit)

	plt.show()

	_serialPort.close()
	return 0


# -------------------------------------------------------------------------
if __name__ == "__main__":
    main(sys.argv[1:])