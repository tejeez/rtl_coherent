#include <complex.h>
#include <fftw3.h>
#include <stdint.h>
#include <time.h>
#include "synchronize.h"

/* for fractional-sample resolution */
#define CORRELATION_OVERSAMPLE 4


static int nreceivers=0, corrlen=0, fft1n=0, fft2n=0;


fftwf_complex *fft1in, *fft1out, *fft2in, *fft2out;
fftwf_plan fft1plan, fft2plan;

int sync_init(int nreceivers_init, int corrlen_init) {
	int i, arraysize;
	nreceivers = nreceivers_init;
	corrlen = corrlen_init;
	
	/* half of fft input will be zero-padded */
	fft1n = corrlen*2;
	fft2n = fft1n * CORRELATION_OVERSAMPLE;
	
	arraysize = nreceivers * fft1n;
	fft1in  = fftwf_malloc(arraysize * sizeof(*fft1in));
	fft1out = fftwf_malloc(arraysize * sizeof(*fft1out));
	for(i = 0; i < arraysize; i++)
		fft1in[i] = fft1out[i] = 0;
		
	arraysize = (nreceivers-1) * fft2n;
	fft2in  = fftwf_malloc(arraysize * sizeof(*fft2in));
	fft2out = fftwf_malloc(arraysize * sizeof(*fft2out));
	for(i = 0; i < arraysize; i++)
		fft2in[i] = fft2out[i] = 0;
	
	fft1plan = fftwf_plan_many_dft(
		1, &fft1n, nreceivers,
		fft1in,  NULL, 1, fft1n,
		fft1out, NULL, 1, fft1n,
		FFTW_FORWARD, FFTW_ESTIMATE);

	fft2plan = fftwf_plan_many_dft(
		1, &fft2n, nreceivers-1,
		fft2in,  NULL, 1, fft2n,
		fft2out, NULL, 1, fft2n,
		FFTW_BACKWARD, FFTW_ESTIMATE);
	return 0;
}


int sync_block(int blocksize, void **buffersv) {
	int ri, i;
	csample_t **buffers = (csample_t**)buffersv;
	if(blocksize < corrlen) return -1;
	
	for(ri = 0; ri < nreceivers; ri++) {
		csample_t *buf = buffers[ri];
		fftwf_complex *floatbuf = fft1in + ri * fft1n;
		for(i = 0; i < corrlen; i++)
			floatbuf[i] = (buf[i][0] + I*buf[i][1]) - (127.4f+127.4f*I);
	}
	fftwf_execute(fft1plan);

	for(ri = 1; ri < nreceivers; ri++) {
		/* cross correlation of receiver number ri and receiver 0 */
		fftwf_complex *f1o = fft1out + ri * fft1n;
		fftwf_complex *f2i = fft2in  + (ri-1) * fft2n;
		/* positive frequencies: */
		for(i = 0; i < fft1n/2; i++)
			f2i[i] = fft1out[i] * conjf(f1o[i]);
		/* negative frequencies: */
		f2i += fft2n - fft1n;
		for(i = fft1n/2; i < fft1n; i++)
			f2i[i] = fft1out[i] * conjf(f1o[i]);
	}
	fftwf_execute(fft2plan);
	printf("%d ",time(NULL));
	for(ri = 1; ri < nreceivers; ri++) {
		fftwf_complex *f2o = fft2out  + (ri-1) * fft2n;
		float maxmagsq = 0, phasedifference;
		float timedifference = 0;
		float complex maxc = 0;
		float y1, y2, y3;
		int maxi = 1;
		for(i = 0; i < fft2n; i++) {
			float complex c = f2o[i];
			float magsq = crealf(c)*crealf(c) + cimagf(c)*cimagf(c);
			if(magsq > maxmagsq) {
				maxmagsq = magsq;
				maxc = c;
				maxi = i;
			}
		}
		/* parabolic interpolation for more fractional sample resolution
		(math from http://dspguru.com/dsp/howtos/how-to-interpolate-fft-peak ) */
		y1 = cabsf(f2o[(maxi-1 + fft2n) % fft2n]);
		y2 = cabsf(maxc);
		y3 = cabsf(f2o[(maxi+1 + fft2n) % fft2n]);
		//printf("%E %E %E   ", y1, y2, y3);

		if(maxi >= fft2n/2) maxi -= fft2n;
		timedifference = maxi;
		timedifference += (y3-y1) / (2*(2*y2-y1-y3));
		timedifference *= 1.0 / CORRELATION_OVERSAMPLE;
		
		
		phasedifference = 57.2957795f * cargf(maxc);
		printf("%9.2f %E %6.2f  ", timedifference, maxmagsq, phasedifference);
	}
	printf("\n");
	fflush(stdout);
	return 0;
}


int sync_exit() {
	/* TODO: free everything */
	return 0;
}

