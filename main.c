#include "dongles.h"
#include "synchronize.h"
#include "correlate.h"
#include <signal.h>
#include <stdlib.h>
#include <time.h>

#define NRECEIVERS_MAX 16

int ndongles = 3, blocksize = 1200000>>10<<10;
int di;
int readfromfile = 0, writetofile = 0, writetofile2 = 0;
samples_t **buffers;
FILE **files, **files2;


volatile int do_exit = 0;
static void sighandler(int signum) {
	(void)signum;
	do_exit = 1;
}
static void initsignals() {
	struct sigaction sigact;
	sigact.sa_handler = sighandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGQUIT, &sigact, NULL);
	sigaction(SIGPIPE, &sigact, NULL);
}


static int dodsp(int blocksize, void **buffers) {
	int nsamples = 0;
	int di;
	csample_t *bufs[NRECEIVERS_MAX];
	float fracdiffs[NRECEIVERS_MAX], phasediffs[NRECEIVERS_MAX];
	sync_blockp(blocksize, buffers, &nsamples, bufs, fracdiffs, phasediffs);
	if(writetofile2) {
		for(di = 0; di < ndongles; di++) {
			fwrite(bufs[di], nsamples*sizeof(**bufs), 1, files2[di]);
		}
	}
	//corr_block(nsamples, bufs, fracdiffs, phasediffs);
	return 0;
}


int main(int argc, char *argv[]) {
	int timestamp = time(NULL);
	if(argc >= 2) {
		if(argv[1][0] == 'r' && argc >= 3) {
			readfromfile = 1;
			timestamp = atoi(argv[2]);
		}
		if(argv[1][0] == 'w')
			writetofile = 1;
		if(argv[1][0] == 'W' || argv[1][1] == 'W')
			writetofile2 = 1;
	}
	initsignals();
	sync_init(ndongles, 16384);
	corr_init(ndongles, 32);

	buffers = malloc(ndongles * sizeof(*buffers));
	files = malloc(ndongles * sizeof(*files));
	files2 = malloc(ndongles * sizeof(*files2));
	for(di=0; di < ndongles; di++) {
		char t[32];
		buffers[di] = malloc(blocksize * sizeof(**buffers));
		if(readfromfile || writetofile) {
			sprintf(t, "d%02d_%d", di, timestamp);
			if((files[di] = fopen(t, readfromfile ? "rb" : "wb")) == NULL) {
				fprintf(stderr, "Could not open file\n");
				goto fail;
			}
		}
		if(writetofile2) {
			sprintf(t, "c%02d_%d", di, timestamp);
			if((files2[di] = fopen(t, "wb")) == NULL) {
				fprintf(stderr, "Could not open file2\n");
				goto fail;
			}
		}
	}
	if(readfromfile) {
		for(;;) {
			for(di = 0; di < ndongles; di++) {
				if(fread(buffers[di], blocksize, 1, files[di]) == 0)
					goto fail;
			}
			dodsp(blocksize, (void**)buffers);
		}
	} else {
		coherent_init(ndongles, 2400000, 434e6, 300);

		while(do_exit == 0) {
			coherent_read(blocksize, buffers);
			dodsp(blocksize, (void**)buffers);
			if(writetofile) {
				for(di = 0; di < ndongles; di++) {
					fwrite(buffers[di], blocksize, 1, files[di]);
				}
			}
		}

		coherent_exit();
	}
	fail:
	sync_exit();
	return 0;
}


