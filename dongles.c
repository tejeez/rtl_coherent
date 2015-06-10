#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <rtl-sdr.h>
#include <complex.h>
#include <math.h>

const int samp_rate=2400000, frequency=25e6, ndongles=4;
const int blocksize=2400000>>10<<10; /* make it aligned to 2^10 bytes */


typedef uint8_t samples_t;
struct dongle_struct {
	int id, blocksize;
	samples_t *buffer;
	FILE *file;
	rtlsdr_dev_t *dev;
	pthread_t dongle_t, control_t;
};
struct dongle_struct *dongles;

pthread_cond_t  dongle_c;
pthread_mutex_t dongle_m;
sem_t dongle_sem;

static volatile int do_exit = 0;
static volatile enum {DONGLE_NOTHING, DONGLE_READ, DONGLE_EXIT} dongle_task = DONGLE_NOTHING;


/* http://stackoverflow.com/questions/6932401/elegant-error-checking */
#define CHECK1(x) do { \
	int retval = (x); \
	if (retval < 0) { \
		fprintf(stderr, "Runtime error: %s returned %d at %s:%d. Closing device.\n", #x, retval, __FILE__, __LINE__); \
		goto err; \
	} \
} while (0)

#define CHECK2(x) do { \
	int retval = (x); \
	if (retval < 0) { \
		fprintf(stderr, "Runtime error: %s returned %d at %s:%d\n", #x, retval, __FILE__, __LINE__); \
	} \
} while (0)

void *dongle_f(void *arg) {
	struct dongle_struct *ds = arg;
	rtlsdr_dev_t *dev = NULL;
	int blocksize = ds->blocksize, n_read = 0;

	fprintf(stderr, "Initializing %d\n", ds->id);
	CHECK1(rtlsdr_open(&dev, ds->id));
	ds->dev = dev;
	CHECK1(rtlsdr_set_sample_rate(dev, samp_rate));
	CHECK1(rtlsdr_set_center_freq(dev, frequency));
	CHECK1(rtlsdr_set_tuner_gain_mode(dev, 0));
	//CHECK1(rtlsdr_set_tuner_gain(dev, 0));
	CHECK1(rtlsdr_reset_buffer(dev));

	//pthread_mutex_lock(&dongle_m);
	sem_post(&dongle_sem);
	fprintf(stderr, "Initialized %d\n", ds->id);
	
	for(;;) {
		int task;
		pthread_mutex_lock(&dongle_m);
		if(dongle_task == DONGLE_EXIT)
			break;
		pthread_cond_wait(&dongle_c, &dongle_m);
		task = dongle_task;
		pthread_mutex_unlock(&dongle_m);

		if(task == DONGLE_READ) {
			int ret;
			n_read = 0;
			errno = 0;
			CHECK2(ret = rtlsdr_read_sync(dev, ds->buffer, blocksize, &n_read));
			sem_post(&dongle_sem);
			if(ret < 0) {
			} else if(n_read < blocksize) {
				fprintf(stderr, "Short read %d: %d/%d\n", ds->id, n_read, blocksize);
			} else {
				fprintf(stderr, "Read %d\n", ds->id);
			}
		}
		if(task == DONGLE_EXIT)
			break;
	}

	err:
	fprintf(stderr, "Exiting %d\n", ds->id);
	//pthread_mutex_unlock(&dongle_m);
	if(dev)
		CHECK2(rtlsdr_close(dev));
	do_exit = 1;
	return NULL;
}


static void sighandler(int signum) {
	(void)signum;
	do_exit = 1;
}


void initsignals() {
	struct sigaction sigact;
	sigact.sa_handler = sighandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGQUIT, &sigact, NULL);
	sigaction(SIGPIPE, &sigact, NULL);
}


#define DEBUGFILES

int initdongles() {
	int di;
	pthread_mutex_init(&dongle_m, NULL);
	pthread_cond_init(&dongle_c, NULL);
	sem_init(&dongle_sem, 0, 0);

	dongles = malloc(sizeof(struct dongle_struct) * ndongles);
	for(di = 0; di < ndongles; di++) {
		struct dongle_struct *d = &dongles[di];
		memset(d, 0, sizeof(struct dongle_struct));
		d->id = di;
		d->blocksize = blocksize;
		d->buffer = malloc(blocksize);
#ifdef DEBUGFILES
		{char t[32];
		sprintf(t, "d%02d", di);
		d->file = fopen(t, "wb");}
#endif
	}
	for(di=0; di < ndongles; di++)
		pthread_create(&dongles[di].dongle_t, NULL, dongle_f, &dongles[di]);
	
	for(di=0; di < ndongles; di++)
		sem_wait(&dongle_sem);
	fprintf(stderr, "All dongles initialized\n");
	
	return 0;
}


int readdongles() {
	int di;
	pthread_mutex_lock(&dongle_m);
	dongle_task = DONGLE_READ;
	pthread_cond_broadcast(&dongle_c);
	pthread_mutex_unlock(&dongle_m);

	for(di=0; di < ndongles; di++)
		sem_wait(&dongle_sem);
	fprintf(stderr, "Read all\n");
	
	return 0;
}


int exitdongles() {
	int di;
	pthread_mutex_lock(&dongle_m);
	dongle_task = DONGLE_EXIT;
	pthread_cond_broadcast(&dongle_c);
	pthread_mutex_unlock(&dongle_m);

	for(di=0; di<ndongles; di++)
		pthread_join(dongles[di].dongle_t, NULL);
	
	pthread_mutex_destroy(&dongle_m);
	pthread_cond_destroy(&dongle_c);
	return 0;
}


int main() {
	int i = 0;
	initsignals();

	initdongles();

	while(do_exit == 0 && i++<2) {
		readdongles();
#ifdef DEBUGFILES
		int di;
		for(di = 0; di < ndongles; di++) {
			struct dongle_struct *d = &dongles[di];
			fwrite(d->buffer, d->blocksize, 1, d->file);
		}
#endif
	}

	exitdongles();
	return 0;
}

