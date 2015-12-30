#include <pthread.h>
#include <rtl-sdr.h>
#include <stdio.h>
typedef uint8_t samples_t;
struct dongle_struct {
	int id, blocksize;
	samples_t *buffer;
	rtlsdr_dev_t *dev;
	pthread_t dongle_t, control_t;
};

int coherent_init();
int coherent_read();
int coherent_exit();

