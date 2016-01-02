#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "configuration.h"

struct configuration conf;

#define LINE_MAX 100

#define CONF_INT(c) if(strcmp(a, #c) == 0) conf.c = atoi(b); else
#define CONF_FLOAT(c) if(strcmp(a, #c) == 0) conf.c = atof(b); else

int conf_read(const char *filename) {
	FILE *f;
	char line[LINE_MAX], a[32], acmp[32], b[32];
	int r, i;

	memset(&conf, 0, sizeof(struct configuration));

	f = fopen(filename, "r");
	if(f == NULL) return -1;

	for(;;) {
		char *rc;
		rc = fgets(line, LINE_MAX, f);
		if(rc == NULL) break;
		if(line[0] == '\0' || line[0] == '#') continue;
		r = sscanf(line, "%31s %31s", a, b);
		if(r == 2) {
			CONF_INT(nreceivers)
			CONF_INT(nbuffers)
			CONF_INT(firstdongle)
			CONF_INT(trigger_id)
			CONF_INT(blocksize)
			CONF_INT(sample_rate)
			CONF_INT(center_freq)
			CONF_INT(if_freq)
			CONF_INT(gain)
			CONF_INT(blocksize)
			
			CONF_INT(sync_start)
			CONF_INT(sync_len)
			CONF_INT(sync_end)
			
			CONF_INT(cor_fft)
			{};
			for(i = 0; i < NRECEIVERS_MAX; i++) {
				snprintf(acmp, 31, "calibdelay%d", i);
				if(strcmp(a, acmp) == 0)
					conf.calibdelay[i] = atof(b);
			}
		}
	}
	fclose(f);
	return 0;
}
