/* type of complex samples */
typedef uint8_t csample_t[2];

int sync_init(int nreceivers, int corrlen);
int sync_block(int blocksize, void **buffers);
int sync_exit();

