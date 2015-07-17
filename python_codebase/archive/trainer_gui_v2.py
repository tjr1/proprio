
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

# GLOBAL DEFIINITIONS
MAX_TIME = 5.


# GLOBAL VARIABLES
fig, ax = plt.subplots()
line, = ax.plot([], [], 'b*:', lw=2)
ax.set_ylim(-1.1, 1.1)
ax.set_xlim(0, MAX_TIME)
ax.grid()
xdata, ydata = [], []

FLAG_PAUSE = False
time_start = datetime.datetime.now()

# Serial Port
_serialPort = serial.Serial()


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
	cnt = serialPortListener.cnt
	profile_tstart = time.time()				# for profiling
	while cnt < 1000:
		if not FLAG_PAUSE:
			cnt += 1
			msg = _serialPort.readline()
			try:
				rx_serial_data = float(msg)
			except ValueError:
				rx_serial_data = rx_serial_data

			time_now = datetime.datetime.now()
			dt = time_now - time_start
			t = dt.seconds + dt.microseconds/1000000.;

			if (cnt % 100 == 0):
				print 'FPS:' , 100/(time.time()-profile_tstart)
				profile_tstart = time.time()				# for profiling

		yield t, rx_serial_data
serialPortListener.cnt = 0
serialPortListener.rx_serial_data = 0

def run(data):
	# update the data
	t,y = data
	xdata.append(t)
	ydata.append(y)
	xmin, xmax = ax.get_xlim()

	if t > MAX_TIME:
		size_xdata = len(xdata)
		cutoff_time = t-MAX_TIME
		xdata = [(x) for x in xdata if x < cutoff_time]
		num_items_cut = size_xdata - len(xdata)
		ydata = ydata[num_items_cut:]

	#if t >= xmax:
	#	ax.set_xlim(xmin, 2*xmax)
	#	ax.figure.canvas.draw()
	line.set_data(xdata, ydata)

	return line,

def exit(event):
	plt.close("all")

# MAIN FUNCTION
def main(argv=None):

	COM_PORT_NUMBER = 10
	_serialPort.baudrate = 9600
	_serialPort.port = COM_PORT_NUMBER-1
	_serialPort.timeout=0.001 
	_serialPort.open()
	if (_serialPort.isOpen() == True):
		print "Opening COM port: " + _serialPort.name
	else:
		print "Error: Unable to open COM port: " + _serialPort.name

	# Make Initial Handshake
	_serialPort.write("Hello World!\n")

	# Initialize Animation Loop
	ani = animation.FuncAnimation(fig, run, serialPortListener, blit=True, interval=1,
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


if __name__ == "__main__":
    main()