#!/usr/bin/python3

# plot covariance matrix


import matplotlib.pyplot as plt
import cmath
import math

n = 3
nn = n*n

plt.figure(1, figsize=(16,12))

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
			try:
				r = 10*math.log10(r) - 30
			except ValueError:
				r = 0

			print(r)
	
			r1 = [0, r]
			ph1 = [ph, ph]
			
			plots[i].set_xdata(ph1)
			plots[i].set_ydata(r1)
		print("\n")
		fifo.close()
		plt.draw()

