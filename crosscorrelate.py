#!/usr/bin/python
# script to test cross correlating saved blocks containing only coherent noise

import math
import cmath
import matplotlib.pyplot as plt
import numpy as np
from scipy import signal

# files having only one received block
files = ["d00_b", "d01_b", "d02_b"]
signals = []
for filename in files:
	a = np.fromfile(filename, dtype=np.dtype("u1"))
	a = a[100000:108192]   # take short block
	a = a.astype(np.float32).view(np.complex64)
	a -= (127.4 + 127.4j)
	signals.append(a)

for i in range(2):
	sig1 = signals[0]
	sig2 = signals[1+i]

	data_length = len(sig1)
	print "data length:", data_length
	b = np.zeros(data_length * 2, dtype=np.complex64)
	b[data_length/2:data_length/2+data_length] = sig1
	corr = signal.fftconvolve(sig1, np.conj(sig2[::-1]))
	#corr = corr[data_length - 5000: data_length + 5000]
	
	peakpos = np.argmax(np.abs(corr))
	print "correlation peak:", peakpos, cmath.polar(corr[peakpos])
	
	plt.subplot(2,1,1+i)
	plt.plot(np.abs(corr))

plt.show()

