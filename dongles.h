#include <pthread.h>
#include <rtl-sdr.h>
#include <stdio.h>
typedef uint8_t samples_t;
struct dongle_struct {
	int id, blocksize;
	samples_t *buffer;
	FILE *file;
	rtlsdr_dev_t *dev;
	pthread_t dongle_t, control_t;
};
extern struct dongle_struct *dongles;

int coherent_init(int ndongles, int blocksize, int samprate, int frequency, int gain);
int coherent_read();
int coherent_exit();

#define DEBUGFILES

