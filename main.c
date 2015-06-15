#include "dongles.h"
#include <signal.h>

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

int main() {
	int i = 0;
	int ndongles = 3;
	initsignals();

	coherent_init(ndongles, 2400000>>10<<10, 2400000, 25e6, 100);

	while(do_exit == 0 && i++<2) {
		coherent_read();
#ifdef DEBUGFILES
		int di;
		for(di = 0; di < ndongles; di++) {
			struct dongle_struct *d = &dongles[di];
			fwrite(d->buffer, d->blocksize, 1, d->file);
		}
#endif
	}

	coherent_exit();
	return 0;
}

