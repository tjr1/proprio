
import matplotlib as mpl
mpl.rcParams['toolbar'] = 'None'

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.widgets import Button

fig, ax = plt.subplots()
line, = ax.plot([], [], lw=2)
ax.set_ylim(-1.1, 1.1)
ax.set_xlim(0, 5)
ax.grid()
xdata, ydata = [], []

FLAG_PAUSE = False;

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

def run(data):
	# update the data
	t,y = data
	xdata.append(t)
	ydata.append(y)
	xmin, xmax = ax.get_xlim()

	if t >= xmax:
		ax.set_xlim(xmin, 2*xmax)
		ax.figure.canvas.draw()
	line.set_data(xdata, ydata)

	return line,

def main(argv=None):
	ani = animation.FuncAnimation(fig, run, data_gen, blit=True, interval=10,
		repeat=False)

	axprev = plt.axes([0.7, 0.05, 0.1, 0.075])
	axnext = plt.axes([0.81, 0.05, 0.1, 0.075])

	bnext = Button(axnext, 'Pause')
	bnext.on_clicked(pause_animation)

	bprev = Button(axprev, 'Play')
	bprev.on_clicked(play_animation)

	plt.show()


if __name__ == "__main__":
    main()