#include <complex.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "configuration.h"
#include "df.h"

//#define NPAIRS_MAX 16
#define NDIRS 64


struct antennapair {
	int ci; /* index in covariance array */
	float x;
	float y;
};

struct df_result {
	int dir;
	float corr;
	float power;
};


static int npairs = 3; /* number of antenna pairs */
static const struct antennapair pairs[/*NPAIRS_MAX*/] = {
{1, 5, 0},
{2, 2.5, 4.33},
{5, -2.5, 4.33}
};
static float complex *expected; /* expected phase offsets per every direction */

static struct df_result *df_results;
static FILE *resultfile;

int df_init() {
	int idir, ipair;
	float lambda = 299792458.0f / conf.center_freq;
	expected = malloc(sizeof(*expected) * NDIRS * npairs);
	df_results = malloc(sizeof(*df_results) * conf.cor_fft);
	for(idir = 0; idir < NDIRS; idir++) {
		float d = 2.0f * M_PI * idir / NDIRS;
		for(ipair = 0; ipair < npairs; ipair++) {
			float x, y, xnew;
			x = pairs[ipair].x;
			y = pairs[ipair].y;
			/* rotate */
			xnew = x * cosf(d) - y * sinf(d);
			/*expected[ai * NDIRS + di].ph = cexpf(I * 2.0f * M_PI * xnew / lambda);*/
			/* Use -phase so we don't need conjugate in correlation.
			   Also divide by 2*npairs to put the correlation value between 0 and 1. */
			expected[ipair * NDIRS + idir] = cexpf(-I * 2.0f * M_PI * xnew / lambda) / (2.0f*npairs);
		}
	}
	resultfile = fopen("fifo", "w");
	return 0;
}




int df_block(float complex *covar) {
	const int ncols = conf.nreceivers * conf.nreceivers;
	const int nbins = conf.cor_fft;
	int idir, ibin, ipair;

	for(ibin = 0; ibin < nbins; ibin++) {
		float complex *cvs = covar + ncols * ibin;
		float maxcorr = 0, power = 0;
		int maxdir = 0;
		float complex cv[npairs];

		for(ipair = 0; ipair < npairs; ipair++) {
			const float complex c = cvs[pairs[ipair].ci];
			const float a = cabsf(c);
			power += a;
			/* normalize */
			cv[ipair] = c / a;
		}

		for(idir = 0; idir < NDIRS; idir++) {
			float correlation = 0.5f; /* to put the resulting value between 0 and 1 */
			for(ipair = 0; ipair < npairs; ipair++) {
				correlation += crealf(expected[ipair * NDIRS + idir] * cv[ipair]);
			}
			if(correlation > maxcorr) {
				maxcorr = correlation;
				maxdir = idir;
			}
		}
		df_results[ibin].dir = maxdir;
		df_results[ibin].corr = maxcorr;
		df_results[ibin].power = power;
#if 0
		if(ibin == 0) {
			printf("%d %E %E\n", maxdir, (double)power, (double)maxcorr);
		}
#endif
	}
	if(resultfile) {
		fwrite("R", 1, 1, resultfile); /* mark beginning of array */
		fwrite(&nbins, sizeof(int), 1, resultfile); /* size of array */
		fwrite(df_results, sizeof(*df_results) * nbins, 1, resultfile);
	}

	return 0;
}

