#include <stdint.h>
#define NRECEIVERS_MAX 16

struct configuration {
	int nreceivers;
	int nbuffers; /* Number of buffers to allocate for the queue between dongle control and DSP threads */

	/* RTL-SDR configuration */
	uint32_t firstdongle; /* rtlsdr index of the first dongle to use */
	int trigger_id; /* dongle whose I2C clock is used to trigger noise burst*/
	uint32_t sample_rate;
	uint32_t center_freq;
	uint32_t if_freq;
	int gain; /* tenths of a dB */
	int blocksize; /* size of blocks to read. multiple of 1024. */


	/* Synchronization */
	int sync_start; /* First sample of block used for synchronization. Sync burst should always have started before this. */
	int sync_len; /* Number of samples used for synchronization. Synchronization burst should never end before point syncstart+synclen. */
	int sync_end; /* First sample used for actual antenna signals. Synchronization burst should always have ended before this. */


	/* Correlation of antenna signals */
	int cor_fft; /* FFT size. Number of frequency bins. */
	float calibdelay[NRECEIVERS_MAX]; /* extra delay to add (in seconds). This is used to correct for cable skew. */

	/* Direction finding */
	float antennax[NRECEIVERS_MAX], antennay[NRECEIVERS_MAX];
};

extern struct configuration conf;

int conf_read(const char *filename);
