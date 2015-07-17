#include <complex.h>
#include <fftw3.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include "correlate.h"

#define PIf 3.14159265f

static int nreceivers=0, fft1n=0, covarsize=0;


static fftwf_complex *fft1in, *fft1out, *covar;
static float *fft1win;
static fftwf_plan fft1plan;

int corr_init(int nreceivers_init, int fft1n_init) {
	int i, arraysize;
	nreceivers = nreceivers_init;
	
	fft1n = fft1n_init;
	
	arraysize = nreceivers * fft1n;
	fft1in  = fftwf_malloc(arraysize * sizeof(*fft1in));
	fft1out = fftwf_malloc(arraysize * sizeof(*fft1out));
	for(i = 0; i < arraysize; i++)
		fft1in[i] = fft1out[i] = 0;
		
	fft1win = fftwf_malloc(fft1n * sizeof(*fft1win));
	for(i = 0; i < fft1n; i++) {
		fft1win[i] = sinf(PIf * (i + 0.5f) / fft1n);
	}
	
	covarsize = nreceivers * nreceivers * fft1n;
	covar = fftwf_malloc(covarsize * sizeof(*covar));

	fft1plan = fftwf_plan_many_dft(
		1, &fft1n, nreceivers,
		fft1in,  NULL, 1, fft1n,
		fft1out, NULL, 1, fft1n,
		FFTW_FORWARD, FFTW_ESTIMATE);

	return 0;
}


int corr_block(int blocksize, csample_t **buffers, float *fracdiffs, float *phasediffs) {
	int ri, ri2, ci, i;
	int si;
	int windowstep = fft1n / 2; /* 50% overlap */
	float cvabssumbest = 0;
	int cvabssumbesti = 0;
	for(ci = 0; ci < covarsize; ci++) covar[ci] = 0;
	
	for(si = 0; si < blocksize-fft1n; si += windowstep) {
		for(ri = 0; ri < nreceivers; ri++) {
			csample_t *buf = buffers[ri] + si;
			fftwf_complex *fftinbuf = fft1in + ri * fft1n;
			for(i = 0; i < fft1n; i++) {
				fftinbuf[i] = ((buf[i][0] + I*buf[i][1]) - (127.4f+127.4f*I)) * fft1win[i];
			}
		}
		fftwf_execute(fft1plan);
		for(ri = 0; ri < nreceivers; ri++) {
			/* TODO phase slope for fractional delay */
			float complex pd = cexpf(I * phasediffs[ri]);
			for(i = 0; i < fft1n; i++)
				fft1out[i] *= pd;
		}
		ci = 0;
		for(i = 0; i < fft1n; i++) {
			for(ri = 0; ri < nreceivers; ri++) {
				for(ri2 = 0; ri2 < nreceivers; ri2++) {
					covar[ci] += fft1out[fft1n*ri + i] * conjf(fft1out[fft1n*ri2 + i]);
					ci++;
				}
			}
		}
	}
	for(i = 0; i < fft1n; i++) {
		float cvabssum = 0;
		for(ri = 0; ri < nreceivers; ri++) {
			for(ri2 = 0; ri2 < nreceivers; ri2++) {
				if(ri != ri2) cvabssum += covar[ci];
				//printf("%E %E  ", crealf(covar[ci]), cimagf(covar[ci]));
				ci++;
			}
			//printf(" ");
		}
		//printf("\n");
		if(cvabssum > cvabssumbest) {
			cvabssumbest = cvabssum;
			cvabssumbesti = i;
		}
	}
	printf("%E %d\n", cvabssumbest, cvabssumbesti);
}


int corr_exit() {
	/* TODO: free everything */
	return 0;
}

