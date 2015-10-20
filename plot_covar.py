#!/usr/bin/python3

# plot covariance matrix


import matplotlib.pyplot as plt
import cmath
import math

n = 3
nn = n*n

plots = []
for i in range(nn):
	sp = plt.subplot(n, n, 1+i, polar=True)
	p, = sp.plot([0, 0], [0, 50])
	plots.append(p)
#plt.show()
plt.show(block = False)



while 1:
	with open("fifo") as fifo:
		for i in range(nn):
			a = fifo.readline()
			co = complex(a)
			r, ph = cmath.polar(co)
			r = 10*math.log10(r) - 30
			print(r)
			
			try:
				r1 = [0, r]
			except ValueError:
				r1 = 0
			ph1 = [ph, ph]
			
			plots[i].set_xdata(ph1)
			plots[i].set_ydata(r1)
		print("\n")
		fifo.close()
		plt.draw()

