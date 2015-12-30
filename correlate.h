/* type of complex samples */
typedef uint8_t csample_t[2];

int corr_init();
int corr_block(int blocksize, csample_t **buffers, float *fracdiffs, float *phasediffs);
int corr_exit();

