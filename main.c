#include "dongles.h"
#include "synchronize.h"
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
	int di;
	int readfromfile = 0, writetofile = 0;
	samples_t **buffers;
	FILE **files;
	if(argc >= 2) {
		if(argv[1][0] == 'r')
			readfromfile = 1;
		if(argv[1][0] == 'w')
			writetofile = 1;
	}
	initsignals();
	sync_init(ndongles, 16384);

	buffers = malloc(ndongles * sizeof(*buffers));
	files = malloc(ndongles * sizeof(*files));
	for(di=0; di < ndongles; di++) {
		char t[32];
		buffers[di] = malloc(blocksize * sizeof(**buffers));
		sprintf(t, "d%02d", di);
		if(readfromfile || writetofile) {
			if((files[di] = fopen(t, readfromfile ? "rb" : "wb")) == NULL) {
				fprintf(stderr, "Could not open file\n");
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
			sync_block(blocksize, (void**)buffers);
		}
	} else {
		coherent_init(ndongles, 2400000, 25e6, 200);

		while(do_exit == 0 /*&& i++<2*/) {
			coherent_read(blocksize, buffers);
			sync_block(blocksize, (void**)buffers);
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


