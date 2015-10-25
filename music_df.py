#!/usr/bin/env python3
# -*- encoding: utf-8 -*-

# plot covariance matrix

import cmath
from itertools import product
import math
import matplotlib.pyplot as plt
import numpy as np
import sys
import time

# Freq
F = 434100000.0
# Measurements in metres
C = 299792458.0
LAMBDA = C / F 

COV_SQUELCH = 500e3

# "x" is the depth of the element in metres when the wavefront is arriving from direction 0 degrees
# "y" is the offset to the side for the element 

# Pizza box equilateral
ANTENNAS = [
	(0.0, 0),
	(0.1992, 0.115),
	(0.1992, -0.115),
]

n = 3
nn = n*n

dirs = np.linspace(0, 2*math.pi, num=360)
# Precalculate expected phase offsets per every direction
steering_vectors = []
for d in dirs:
	angles = []
	for x, y in ANTENNAS:
		#rotate
		xnew = x * math.cos(d) - y * math.sin(d)
		# ynew = y * math.cos(d) + y * math.sin(d)
		pointer = np.exp(1j * xnew / LAMBDA * 2 * math.pi)
		angles.append(pointer)
	steering_vector = np.matrix(angles).H
	steering_vectors.append((d, steering_vector, steering_vector.H))

#print(steering_vectors)


plt.figure(1, figsize=(16,12))

plots = []
sp = plt.subplot(1, 1, 1, polar=True)
p, = sp.plot([0, 0], [0, n/2], linewidth=2)
maxind, = sp.plot([0, 0], [0, n/2], linewidth=5)
plots.append(p)
#plt.show()
plt.show(block = False)

while 1:
	with open("fifo") as fifo:
	
		# Read n**2 values into a matrix. Failing
		# that, try again.
		cov = np.empty((n, n), dtype=complex)
		try:
			for i, j in product(range(n), range(n)):
				co_str = fifo.readline()
				co = complex(co_str)
				cov[i, j] = co
		except ValueError:
			# print("Erroneus complex number:", co_str)
			continue
		
		evals, evecs = np.linalg.eigh(cov)

		print(evals)
		
		# Check if there are any large enough eigenvalues (signals)
		signals = []
		for e_index, e in enumerate(evals):
			if e > COV_SQUELCH:
				signals.append((e_index, e))
		if len(signals) == 0:
			fifo.close()
			continue

		noise_evecs = np.matrix(np.delete(
			evecs, [x for x, _ in signals], 1))

		"""
		print("N:")
		print(noise_evecs)
		print()
		print("N×N.H:")
		print()
		print(noise_evecs * noise_evecs.H)
		print()
		"""		

		max_dir = 0
		max_value = 0
		foos = []
		for d, s, sh in steering_vectors:
			foo = sh * noise_evecs * noise_evecs.H * s
			quality = 1 / abs(foo[0, 0])
			if quality > max_value:
				max_dir = d
				max_value = quality
			foos.append(1 / abs(foo[0, 0]))

		max_dir_xdata = [max_dir, max_dir]
		max_dir_ydata = [0, max_value]
		
		plots[0].set_xdata(dirs)
		plots[0].set_ydata(foos)
		maxind.set_xdata(max_dir_xdata)
		maxind.set_ydata(max_dir_ydata)

		fifo.close()
		plt.draw()

