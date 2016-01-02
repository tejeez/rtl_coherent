#include "configuration.h"
#include "dongles.h"
#include "synchronize.h"
#include "correlate.h"
#include "df.h"
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int ndongles = 0, blocksize = 0, nbuffers = 0;
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
	sync_blockp(blocksize / sizeof(csample_t), (csample_t**)buffers, &nsamples, bufs, fracdiffs, phasediffs);
	if(writetofile2) {
		for(di = 0; di < ndongles; di++) {
			fwrite(bufs[di], nsamples*sizeof(**bufs), 1, files2[di]);
		}
	}
	corr_block(nsamples, bufs, fracdiffs, phasediffs);
	return 0;
}



/*
DSP and dongle control are done in separate threads to make it possible to be
reading the next block while the previous block is being processed.
There's a simple lockless queue between the threads.
*/
static volatile int nbuf_write=0, nbuf_read=0;
pthread_t dspthread;

static void *dspthread_f(void *arg) {
	int nbuf_r = nbuf_read;
	(void)arg;
	while(!do_exit) {
		while(nbuf_r != nbuf_write) { /* data waiting in buffers? */
			dodsp(blocksize, (void**)(buffers + ndongles * nbuf_r));
			if(writetofile) {
				for(di = 0; di < ndongles; di++) {
					fwrite(buffers[di], blocksize, 1, files[di]);
				}
			}
			nbuf_read = nbuf_r = (nbuf_r + 1) % nbuffers;
		}
		usleep(100000); /* TODO: proper synchronization */
	}
	return NULL;
}


int main(int argc, char *argv[]) {
	int timestamp = time(NULL);
	if(argc < 2) {
	}
	if(argc >= 3) {
		if(argv[2][0] == 'r' && argc >= 4) {
			readfromfile = 1;
			timestamp = atoi(argv[3]);
		}
		if(argv[2][0] == 'w')
			writetofile = 1;
		if(argv[2][0] == 'W' || argv[2][1] == 'W')
			writetofile2 = 1;
	}
	initsignals();
	conf_read(argv[1]);
	
	/* TODO: get rid of these and make all the code read the configuration struct? */
	ndongles = conf.nreceivers;
	nbuffers = conf.nbuffers;
	blocksize = conf.blocksize;

	sync_init();
	corr_init();
	df_init();

	buffers = malloc(nbuffers * ndongles * sizeof(*buffers));
	files = malloc(ndongles * sizeof(*files));
	files2 = malloc(ndongles * sizeof(*files2));
	
	for(di=0; di < ndongles * nbuffers; di++)
		buffers[di] = malloc(blocksize * sizeof(**buffers));
	
	for(di=0; di < ndongles; di++) {
		char t[32];
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
		int donglesok = coherent_init(ndongles);
		pthread_create(&dspthread, NULL, dspthread_f, NULL);

		if(donglesok == ndongles) {
			while(do_exit == 0) {
				int nbuf_write_next = (nbuf_write + 1) % nbuffers;
				if(nbuf_write_next != nbuf_read) { /* buffers free? */
					if(coherent_read(blocksize, buffers + ndongles * nbuf_write) == -1) break;
					nbuf_write = nbuf_write_next;
				} else {
					fprintf(stderr,"\rBuffer overflow!");
					usleep(100000); /* TODO: proper synchronization */
				}

			}
		}
		coherent_exit();

		pthread_kill(dspthread, SIGINT);
		pthread_join(dspthread, NULL);

	}
	fail:
	sync_exit();
	return 0;
}


