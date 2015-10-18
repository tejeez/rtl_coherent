/* type of complex samples */
typedef uint8_t csample_t[2];

int sync_init(int nreceivers, int corrlen);
int sync_block(int blocksize_samples, csample_t **buffers, float *timediffs, float *phasediffs);
int sync_blockp(int blocksize_samples, csample_t **buffers, int *nsamples_ret, csample_t **buffersret, float *fracdiffs, float *phasediffs);
int sync_exit();

