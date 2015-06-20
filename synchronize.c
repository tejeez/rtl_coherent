#include <complex.h>
#include <fftw3.h>
#include <stdint.h>
#include <time.h>
#include "synchronize.h"


static int nreceivers=0, corrlen=0, fftn=0;


fftwf_complex *fft1in, *fft1out, *fft2in, *fft2out;
fftwf_plan fft1plan, fft2plan;

int sync_init(int nreceivers_init, int corrlen_init) {
	int i, arraysize;
	nreceivers = nreceivers_init;
	corrlen = corrlen_init;
	
	/* half of fft input will be zero-padded */
	fftn = corrlen*2;
	
	arraysize = nreceivers * fftn;
	fft1in  = fftwf_malloc(arraysize * sizeof(*fft1in));
	fft1out = fftwf_malloc(arraysize * sizeof(*fft1out));
	for(i = 0; i < arraysize; i++)
		fft1in[i] = fft1out[i] = 0;
		
	arraysize = (nreceivers-1) * fftn;
	fft2in  = fftwf_malloc(arraysize * sizeof(*fft2in));
	fft2out = fftwf_malloc(arraysize * sizeof(*fft2out));
	for(i = 0; i < arraysize; i++)
		fft2in[i] = fft2out[i] = 0;
	
	fft1plan = fftwf_plan_many_dft(
		1, &fftn, nreceivers,
		fft1in,  NULL, 1, fftn,
		fft1out, NULL, 1, fftn,
		FFTW_FORWARD, FFTW_ESTIMATE);

	fft2plan = fftwf_plan_many_dft(
		1, &fftn, nreceivers-1,
		fft2in,  NULL, 1, fftn,
		fft2out, NULL, 1, fftn,
		FFTW_BACKWARD, FFTW_ESTIMATE);
	return 0;
}


int sync_block(int blocksize, void **buffersv) {
	int ri, i;
	csample_t **buffers = (csample_t**)buffersv;
	if(blocksize < corrlen) return -1;
	
	for(ri = 0; ri < nreceivers; ri++) {
		csample_t *buf = buffers[ri];
		fftwf_complex *floatbuf = fft1in + ri * fftn;
		for(i = 0; i < corrlen; i++)
			floatbuf[i] = (buf[i][0] + I*buf[i][1]) - (127.4f+127.4f*I);
	}
	fftwf_execute(fft1plan);

	for(ri = 1; ri < nreceivers; ri++) {
		/* cross correlation of receiver number ri and receiver 0 */
		fftwf_complex *f1o = fft1out + ri * fftn;
		fftwf_complex *f2i = fft2in  + (ri-1) * fftn;
		for(i = 0; i < fftn; i++)
			f2i[i] = fft1out[i] * conjf(f1o[i]);
	}
	fftwf_execute(fft2plan);
	printf("%d ",time(NULL));
	for(ri = 1; ri < nreceivers; ri++) {
		fftwf_complex *f2o = fft2out  + (ri-1) * fftn;
		float maxmagsq = 0, phasedifference;
		float complex maxc = 0;
		int maxi = 0;
		for(i = 0; i < fftn; i++) {
			float complex c = f2o[i];
			float magsq = crealf(c)*crealf(c) + cimagf(c)*cimagf(c);
			if(magsq > maxmagsq) {
				maxmagsq = magsq;
				maxc = c;
				maxi = i;
			}
		}
		if(maxi >= fftn/2) maxi -= fftn;
		phasedifference = 57.2957795f * cargf(maxc);
		printf("%5d %E %6.2f   ", maxi, maxmagsq, phasedifference);
	}
	printf("\n");
	fflush(stdout);
	return 0;
}


int sync_exit() {
	/* TODO: free everything */
	return 0;
}

