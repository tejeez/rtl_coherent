#include "dongles.h"
#include <signal.h>
#include <stdlib.h>

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

int main(int argc, char *argv[]) {
	int ndongles = 3, blocksize = 2400000>>10<<10;
	int i = 0, di;
	int readfromfile = 0, writetofile = 1;
	samples_t **buffers;
	FILE **files;
	if(argc >= 2) {
		if(argv[1][0] == 'r') {
			readfromfile = 1;
			writetofile = 0;
		}
	}
	initsignals();

	buffers = malloc(ndongles * sizeof(*buffers));
	files = malloc(ndongles * sizeof(*files));
	for(di=0; di < ndongles; di++) {
		char t[32];
		buffers[di] = malloc(blocksize * sizeof(**buffers));
		sprintf(t, "d%02d", di);
		if(readfromfile || writetofile) {
			files[di] = fopen(t, readfromfile ? "rb" : "wb");
		}
	}
	if(readfromfile) {
		for(di = 0; di < ndongles; di++) {
			fread(buffers[di], blocksize, 1, files[di]);
		}
		/* do something with the signals */
	} else {
		coherent_init(ndongles, 2400000, 25e6, 200);

		while(do_exit == 0 /*&& i++<2*/) {
			coherent_read(blocksize, buffers);
			if(writetofile) {
				for(di = 0; di < ndongles; di++) {
					fwrite(buffers[di], blocksize, 1, files[di]);
				}
			}
		}

		coherent_exit();
	}
	return 0;
}

