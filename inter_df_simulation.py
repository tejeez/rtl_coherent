#!/usr/bin/env python3
# -*- encoding: utf-8 -*-

# plot covariance matrix

import matplotlib.pyplot as plt
import cmath
import math
import numpy
import sys
import time

# Freq
F = 434e6
C = 299792458.0
LAMBDA = C / F 

nantennas = 3
radius = 0.34
antennapos = []
for angle in numpy.linspace(0, 2*math.pi, num=nantennas, endpoint=False):
	print(angle)
	antennapos.append((radius*numpy.cos(angle), radius*numpy.sin(angle)))

print(antennapos)

# antenna pairs
ANTENNAS = []
for i in range(nantennas):
	for j in range(i+1,nantennas):
		ANTENNAS.append((nantennas*i+j, antennapos[i][0] - antennapos[j][0], antennapos[i][1] - antennapos[j][1]))

print(ANTENNAS)
#exit(0)
invertphases = 1

indices = [ind for ind, _, _ in ANTENNAS]

n = len(ANTENNAS)
print("n=",n)
nn = n*n

dirs = numpy.linspace(0, 2*math.pi, num=360)
# Precalculate expected phase offsets per every direction
expected_phases = []
for d in dirs:
	phases_per_ant = []
	for _, x, y in ANTENNAS:
		#rotate
		xnew = x * math.cos(d) - y * math.sin(d)
		# ynew = y * math.cos(d) + y * math.sin(d)
		# Ugly capping of phase but it's done only once
		expected_phase = invertphases*cmath.phase(cmath.rect(1, xnew / LAMBDA * 2 * math.pi))
		phases_per_ant.append(expected_phase)
	expected_phases.append(phases_per_ant)

#print(expected_phases)

plt.figure(1, figsize=(16,12))

plots = []
sp = plt.subplot(1, 1, 1, polar=True)
p, = sp.plot([0, 0], [0, (n-0)*2], linewidth=10)
maxind, = sp.plot([0, 0], [0, (n-0)*2], linewidth=10)
plots.append(p)
plt.ion()
plt.show()

while 1:
	for phases in expected_phases:
		corrs = []
		max_index = 0
		max_value = 0
		for dir_index, expected_phase in enumerate(expected_phases):
			# First antenna always correlates with itself fully, so 1
			correlation = 0
			for antenna_index, antenna_phase in enumerate(expected_phase):
				phasediff = antenna_phase - phases[antenna_index]
				correlation += math.cos(phasediff)+1
			corrs.append(correlation)
			if correlation > max_value:
				max_value = correlation
				max_index = dir_index
		max_dir = dirs[max_index]
		max_dir_xdata = [max_dir, max_dir]
		max_dir_ydata = [0, max_value]
		
		plots[0].set_xdata(dirs)
		plots[0].set_ydata(corrs)
		maxind.set_xdata(max_dir_xdata)
		maxind.set_ydata(max_dir_ydata)
		plt.pause(0.01)

