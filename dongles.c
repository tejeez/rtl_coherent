#include <errno.h>
#include <string.h>
#include <semaphore.h>
#include "dongles.h"

static int ndongles=0, samprate=0, frequency=0, blocksize=0, gain=0;
static volatile int donglesok=0;

static pthread_cond_t  dongle_c;
static pthread_mutex_t dongle_m;
static sem_t dongle_sem;

static volatile enum {DONGLE_NOTHING, DONGLE_READ, DONGLE_EXIT} dongle_task = DONGLE_NOTHING;

struct dongle_struct *dongles;

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

static void *dongle_f(void *arg) {
	struct dongle_struct *ds = arg;
	rtlsdr_dev_t *dev = NULL;
	int blocksize = ds->blocksize, n_read = 0;

	fprintf(stderr, "Initializing %d\n", ds->id);
	CHECK1(rtlsdr_open(&dev, ds->id));
	ds->dev = dev;
	CHECK1(rtlsdr_set_sample_rate(dev, samprate));
	CHECK1(rtlsdr_set_dithering(dev, 0));
	CHECK1(rtlsdr_set_center_freq(dev, frequency));
	CHECK1(rtlsdr_set_tuner_gain_mode(dev, 1));
	CHECK1(rtlsdr_set_tuner_gain(dev, gain));
	CHECK1(rtlsdr_reset_buffer(dev));

	fprintf(stderr, "Initialized %d\n", ds->id);
	
	donglesok++;
	for(;;) {
		int task;
		pthread_mutex_lock(&dongle_m);
		if(dongle_task == DONGLE_EXIT)
			break; 
		sem_post(&dongle_sem);
		pthread_cond_wait(&dongle_c, &dongle_m);
		task = dongle_task;
		pthread_mutex_unlock(&dongle_m);

		if(task == DONGLE_READ) {
			int ret;
			n_read = 0;
			errno = 0;
			CHECK2(ret = rtlsdr_read_sync(dev, ds->buffer, blocksize, &n_read));
			if(ret < 0) {
			} else if(n_read < blocksize) {
				fprintf(stderr, "Short read %d: %d/%d\n", ds->id, n_read, blocksize);
			} else {
				fprintf(stderr, "Read %d\n", ds->id);
			}
		} else if(task == DONGLE_EXIT)
			break;
	}
	donglesok--;

	err:
	fprintf(stderr, "Exiting %d\n", ds->id);
	if(dev)
		CHECK2(rtlsdr_close(dev));
	sem_post(&dongle_sem);
	return NULL;
}


int coherent_init(int init_ndongles, int init_blocksize, int init_samprate, int init_frequency, int init_gain) {
	int di;
	ndongles = init_ndongles;
	blocksize = init_blocksize;
	samprate = init_samprate;
	frequency = init_frequency;
	gain = init_gain;
	pthread_mutex_init(&dongle_m, NULL);
	pthread_cond_init(&dongle_c, NULL);
	sem_init(&dongle_sem, 0, 0);

	donglesok = 0;
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
	if(donglesok == ndongles)
		fprintf(stderr, "All dongles initialized\n");
	else
		fprintf(stderr, "%d/%d dongles initialized\n", donglesok, ndongles);
	
	return donglesok;
}


int coherent_read() {
	int di;
	pthread_mutex_lock(&dongle_m);
	dongle_task = DONGLE_READ;
	pthread_cond_broadcast(&dongle_c);
	pthread_mutex_unlock(&dongle_m);

	for(di=0; di < /*ndongles*/donglesok; di++)
		sem_wait(&dongle_sem);
	fprintf(stderr, "Read all\n");
	
	return 0;
}


int coherent_exit() {
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


