#include "main.h"
#include "timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <inttypes.h>

#define ACTI(k) (action[2*(k)])
#define ACTJ(k) (action[2*(k)+1])

static uint32_t nv, ne, naction;
static uint32_t * restrict off;
static uint32_t * restrict from;
static uint32_t * restrict ind;
static uint32_t * restrict weight;
static uint32_t * restrict action;

/* handles for I/O memory */
static uint32_t * restrict graphmem;
static uint32_t * restrict actionmem;

static double * update_time_trace;


#define LDB(...) //printf(__VA_ARGS__);


#define LINE_SIZE 100000

void testVS(uint32_t nv, uint32_t* off, uint32_t* ind);
void testBFS(uint32_t* off, uint32_t* ind);

int main (const int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: bfs <graph-name>\n");
		exit(EXIT_FAILURE);
	}
	char line[LINE_SIZE];

	FILE * fp = fopen(argv[1], "r");
	fgets(line, LINE_SIZE, fp);
	sscanf(line, "%d %d", &nv, &ne);
	nv += 1;
	off = memalign(64, (nv+2) * sizeof(uint32_t));
	ind = memalign(64, (ne*2) * sizeof(uint32_t));
	off[0]=0;
	off[1]=0;
	uint32_t counter=0;
	for (uint32_t u = 1; fgets(line, LINE_SIZE, fp); u++) {

		uint64_t neigh=0;
		uint64_t v = 0;
		char * ptr = line;
		int read = 0;

		while (sscanf(ptr, "%" SCNu64 "%n", &v, &read) > 0) {
			ptr += read;
			neigh++;
			ind[counter++]=v;
		}
		off[u+1]=off[u]+neigh;
	}


	fclose(fp);

        testBFS(off,ind);
 	free(off);
	free(ind);
}

#define INIT_LEVEL_ARRAY(arr) \
	for (size_t i = 0; i < nv; i++) { \
		arr[i] = INT32_MAX; \
	}

void testBFS(uint32_t* off, uint32_t* ind) {
	uint32_t* Queue = (uint32_t*)memalign(64, nv * sizeof(uint32_t));
	uint32_t* level = (uint32_t*)memalign(64, nv * sizeof(uint32_t));
	uint32_t* level2 = (uint32_t*)memalign(64, nv * sizeof(uint32_t));
	
	INIT_LEVEL_ARRAY(level)
	tic();
	BFSSeq(off, ind, Queue, level, 1);
	printf("BFS regular:                               %lf\n", toc() * 1.0e+9);

	INIT_LEVEL_ARRAY(level2)
	tic ();
	BFSSeqBranchless(off, ind, Queue, level2, 1);
	printf("BFS branchless:                            %lf\n", toc() * 1.0e+9);

#ifndef __MIC__
	INIT_LEVEL_ARRAY(level2)
	tic ();
	BFSSeqBranchlessAsm(off, ind, Queue, level2, 1);
	printf("BFS branchless Asm:                        %lf\n", toc() * 1.0e+9);
#endif

#ifdef __SSE__
	INIT_LEVEL_ARRAY(level2)
	tic ();
	BFSSeqBranchlessSSE(off, ind, Queue, level2, 1);
	printf("BFS branchless SSE:                        %lf\n", toc() * 1.0e+9);
#endif

#ifdef __MIC__
	INIT_LEVEL_ARRAY(level2)
	tic ();
	BFSSeqBranchlessMICPartVec(off, ind, Queue, level2, 1);
	printf("BFS branchless MIC (Partially Vectorized): %lf\n", toc() * 1.0e+9);

	INIT_LEVEL_ARRAY(level2)
	tic ();
	BFSSeqBranchlessMICFullVec(off, ind, Queue, level2, 1);
	printf("BFS branchless MIC (Fully Vectorized):     %lf\n", toc() * 1.0e+9);
#endif

	free(Queue);
	free(level);
	free(level2);
}
