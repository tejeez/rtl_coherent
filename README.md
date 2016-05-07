rtl_coherent: Synchronized RTL-SDR receivers
============================================

This is a hardware and software project to synchronize multiple RTL-SDR
receivers and make it possible to use them for applications such as
radio direction finding, passive radars, measuring equipment,
radio astronomy interferometers and MIMO communications.


Basic idea
----------

A single 28.8 MHz reference clock is distributed to all dongles. This makes
their sampling rates and local oscillator frequencies equal but doesn't
guarantee that they would actually sample simultaneously or that their
local oscillators would be in the same phase. 

Both the local oscillator phase and sample time offset get a random value
every time the dongles are initialized. This happens because commands don't
arrive to all receivers at exactly the same time and because their frequency
synthesizers are not known to provide a way to reset their phases.

To handle this we have to find the time and phase offsets every time the
receiver is used. This is done by disconnecting receivers from their antennas
and connecting them to a single white noise source. Cross correlating this
noise finds these offsets and lets us correct them. Currently the signal is
recorded in short blocks and each block starts with a burst of noise.


Electronics
-----------

The current hardware prototype has 3 dongles. One of them has the original
28.8 MHz crystal in place and other 2 dongles have the crystal removed. Clock
is distributed from one dongle to crystal pins of the two other dongles
through small capacitors. A better solution for a larger number of dongles
would be to have a separate 28.8 MHz oscillator and distribute it.

The inputs of the dongles are switched between antennas and noise source by
SA630D switches. They are controlled by an RC timing circuit triggered by
I2C clock in the dongles. Every time the R820T tuner chip receives a
command from the RTL2832U chip, there's some activity on the I2C bus.
This switches the inputs to the noise source and the timing circuit keeps
them there for some time after the I2C traffic has finished. The idea is to
have the noise burst triggered every time the center frequency is changed
which should make fast scanning easier.

I haven't even drawn a proper schematic of the whole hardware but I'll try
to draw and publish it soon.


Software
--------

The current software is primarily written for radio direction finding based on
phase difference between elements of an antenna array.

The software records a block of signal from each dongle, cross correlates the
noise in beginning of each block to determine the phase and timing offsets,
corrects for them, divides the signal in narrow frequency bins using FFT and
calculates covariance between each receiver pair for each frequency bin.
Complex argument of an element in the covariance matrix represents the measured
phase difference between two antennas and is used for direction finding.
The same covariance matrix could also be used in more sophisticated direction
finding algorithms such as MUSIC or ESPRIT which might become more useful when
the number of antennas is increased.

rtl_coherent uses features in keenerd's experimental experimental RTL-SDR
branch. Get it from https://github.com/keenerd/rtl-sdr

The software has mostly been developed in Debian Linux on ordinary x86-64 PCs
but many other Linux distributions and architectures should work as well.


Experiments and demonstrations
------------------------------

Direction finding was first tested on 433 MHz amateur radio band using an array
of 3 groundplane antennas spaced by 1/3 wavelength.
See a video here: https://www.youtube.com/watch?v=8Wzb1mgZ0EE

It was also tested on higher HF bands using an array of ground mounted
vertical antennas. A spectrogram from 26.4 MHz to 28.8 MHz where color hue
represents the estimated angle of arrival can be seen here:
http://prkele.prk.tky.fi/~peltolt2/colordf_27600kHz_7h.jpg
