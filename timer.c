#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <unistd.h>
#include <time.h>

static struct timespec tic_ts;

void tic(void) {
	clock_gettime(CLOCK_MONOTONIC, &tic_ts);
}

double toc(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ((double)ts.tv_nsec - (double)tic_ts.tv_nsec) * 1.0e-9 + ((double)ts.tv_sec - (double)tic_ts.tv_sec);
}
