import pylab as pl
import threading as thrd
import numpy as np
import signal
import time
 
# Reocrd start time for calculating frame rate
tstart = time.time()
 
# SIGINT handling is required for the main thread to catch
# keyboard interrupts
def sigint(signum, frame):
	print 'FPS:' , 200/(time.time()-tstart)
	import sys
	sys.exit()
 
signal.signal(signal.SIGINT, sigint)
 
# Start interactive mode
pl.ion()
 
# Initialize lock semaphore
lock = thrd.Lock()
 
# Initial plot
x = np.arange(0,2*3.14159,0.01)
line, = pl.plot(x,np.sin(x))
 
def Update():
	# Thread for updating plot
	while True:
		lock.acquire()
		pl.draw()
		lock.release()
		time.sleep(0.3)
	return
 
def Set():
	# Thread for setting plot data
	i = 0
	while True:
		lock.acquire()
		#line.set_ydata(np.sin(x+i/10.0))
		pl.plot(np.sin(x+i/10.0))
		lock.release()
		i += 1
		time.sleep(0.6)
	return
 
# Init and start threads
t0 = thrd.Thread(target=Set)
t1 = thrd.Thread(target=Update)
t0.start()
t1.start()
 
# Main loop is necessary for exiting properly with keyboard
# interrupts
while True:
	pass