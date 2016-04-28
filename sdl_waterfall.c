/*
A small quickly written program to plot a waterfall where color tells the direction.
The code is quite ugly but it works.
*/

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <SDL/SDL.h>

#define NBINS_MAX 8192
#define NDIRS 64

struct df_result {
	int dir;
	float corr;
	float power;
};


SDL_Surface *sdls;
void savebmp() {
	int t = time(NULL);
	char fn[100];
	if(sdls == NULL) return;
	sprintf(fn,"img/%d.bmp", t);
	SDL_SaveBMP(sdls, fn);
}



int main() {
	SDL_Event sdle;
	FILE *resultfile;
	struct df_result *df_results;
	int screenw = 1024, screenh = 400;
	int sy = screenh-1;

	float hues[NDIRS][3];
	int i;
	/* precalculate values for conversion from "HSV" */
	/* red to yellow */
	for(i = 0; i < NDIRS/6; i++) {
		float h = (float)i / (NDIRS/6);
		hues[i][0] = 0;
		hues[i][1] = h;
		hues[i][2] = 1;
	}
	/* yellow to green */
	for(; i < NDIRS*2/6; i++) {
		float h = (float)(i - NDIRS/6) / (NDIRS/6);
		hues[i][0] = 0;
		hues[i][1] = 1;
		hues[i][2] = 1.f-h;
	}
	/* green to cyan */
	for(; i < NDIRS*3/6; i++) {
		float h = (float)(i - NDIRS*2/6) / (NDIRS/6);
		hues[i][0] = h;
		hues[i][1] = 1;
		hues[i][2] = 0;
	}
	/* cyan to blue */
	for(; i < NDIRS*4/6; i++) {
		float h = (float)(i - NDIRS*3/6) / (NDIRS/6);
		hues[i][0] = 1;
		hues[i][1] = 1.f-h;
		hues[i][2] = 0;
	}
	/* blue to magenta */
	for(; i < NDIRS*5/6; i++) {
		float h = (float)(i - NDIRS*4/6) / (NDIRS/6);
		hues[i][0] = 1;
		hues[i][1] = 0;
		hues[i][2] = h;
	}
	/* magenta to red */
	for(; i < NDIRS; i++) {
		float h = (float)(i - NDIRS*5/6) / (NDIRS/6);
		hues[i][0] = 1.f-h;
		hues[i][1] = 0;
		hues[i][2] = 1;
	}


	resultfile = fopen("fifo", "r");
	df_results = malloc(sizeof(*df_results) * NBINS_MAX);
	if(resultfile == NULL || df_results == NULL) return 1;

	
	SDL_Init(SDL_INIT_VIDEO);
	sdls = SDL_SetVideoMode(screenw, screenh, 32, SDL_SWSURFACE);
	if(sdls == NULL) goto fail;

	for(;;) {
		int x, x2, binspp, ww;
		int c, nbins=0;
		/* find start of next array */
		do {
			c = fgetc(resultfile);
			if(c < 0) {
				fprintf(stderr, "End of file!\n");
				goto ready;
			}
		} while(c != 'R');
		/* read size of array */
		fread(&nbins, sizeof(int), 1, resultfile);
		if(nbins <= 0 || nbins > NBINS_MAX) {
			fprintf(stderr, "Read invalid nbins\n");
			continue;
		}
		fread(df_results, nbins*sizeof(*df_results), 1, resultfile);
		
		/* bins per pixel. round up */
		binspp = (nbins + screenw-1) / screenw;
		/* width of waterfall drawn */
		ww = nbins / binspp;

		/* Calculate an "average" power over a few bins.
		   This is used to "flatten" slow changes in noise floor versus frequency. */
		float avgpower[ww];
		float avgfilter = 0;
		for(x = 0; x < ww; x++) {
			float powersum = 0;
			for(x2 = 0; x2 < binspp; x2++) {
				struct df_result res;
				res = df_results[(x * binspp + x2 + nbins/2)%nbins];
				//powersum += logf(res.power);
				/* Let's try to use inverse values to put most weight
				   on the bins with least power. It seems to be even better
				   than logarithm. */
				powersum += 1.0f / res.power;
			}
			avgfilter = 0.95f * avgfilter + 0.05f * powersum / (float)binspp;
			avgpower[x] = avgfilter;
		}
		/* Run the same filter backwards to make its impulse response symmetric. */
		avgfilter = 0;
		for(x = ww-1; x >= 0; x--) {
			avgfilter = 0.95f * avgfilter + 0.05f * avgpower[x];
			avgpower[x] = avgfilter;
		}

		/* Draw a result line in the waterfall */
		Uint8 *p = sdls->pixels + sdls->pitch * sy;
		for(x = 0; x < ww; x++) {
			struct df_result maxres = {0,0,0};
			for(x2 = 0; x2 < binspp; x2++) {
				struct df_result res;
				res = df_results[(x * binspp + x2 + nbins/2)%nbins];
				/* find the bin with best DF result */
				/*if(res.corr > maxres.corr) {*/
				/* find the bin with most power */
				if(res.power > maxres.power) {
					maxres = res;
				}
			}
			if(maxres.dir < 0 || maxres.dir >= NDIRS) {
				fprintf(stderr, "Read invalid direction\n");
				break;
			}

			//float brightness = ((logf(maxres.power) - avgpower[x]) + 2.0f) * 50.0f;
			float brightness = ((logf(maxres.power * avgpower[x])) + 0.5f) * 80.0f;
			//printf("%f %f\n", maxres.power, avgpower[x]);
			if(brightness < 0) brightness = 0;
			if(brightness > 255.0f) brightness = 255.0f;
			float saturation = (maxres.corr - 0.75f) * 4.0f;
			if(saturation < 0) saturation = 0;
#if 0
			/* test palette */
			brightness = 200.0f;
			maxres.dir = NDIRS * x / ww;
			saturation = 1;
#endif

			float gray = 0.5f * (1.0f - saturation);
			saturation *= brightness;
			gray *= brightness;
			for(i = 0; i < 3; i++) {
				float v = saturation * hues[maxres.dir][i] + gray;

				/* clip */
				if(v > 255) p[i] = 255;
				else if(v < 0) p[i] = 0;
				else p[i] = v;
			}
			p += 4;
		}
		
		sy--;
		if(sy < 0) {
			sy = screenh-1;
			savebmp();
		}

		// Draw a red indicator line where the next sample will be
		p = sdls->pixels + sdls->pitch * sy;
		for(x = 0; x < ww * 4; x += 4) {
			p[x] = 0;
			p[x + 1] = 0;
			p[x + 2] = 255;
			p[x + 3] = 0; // Alpha? Needed?
		}

		SDL_Flip(sdls);

		while(SDL_PollEvent(&sdle)) {
			if(sdle.type == SDL_QUIT) goto fail;
		}
	}
	ready:
	for(;;) {
		SDL_WaitEvent(&sdle);
		if(sdle.type == SDL_QUIT) goto fail;
	}

	fail:
	savebmp();
	SDL_Quit();
	return 0;
}

