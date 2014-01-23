#include "main.h"
#include "timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <inttypes.h>

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

void testSV(size_t nv, uint32_t* off, uint32_t* ind);
void testBFS(uint32_t* off, uint32_t* ind);

#define LINE_SIZE 10000

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
#if defined(BENCHMARK_BFS)
        testBFS(off,ind);
#endif
#if defined(BENCHMARK_SV)
        testSV(nv, off,ind);
#endif
 	free(off);
	free(ind);
}

#define INIT_LEVEL_ARRAY(arr) \
	for (size_t i = 0; i < nv; i++) { \
		arr[i] = INT32_MAX; \
	}

#if defined(BENCHMARK_BFS)
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

	#ifdef __AVX2__
		INIT_LEVEL_ARRAY(level2)
		tic ();
		BFSSeqBranchlessAVX2(off, ind, Queue, level2, 1);
		printf("BFS branchless AVX2:                       %lf\n", toc() * 1.0e+9);
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
#endif

#if defined(BENCHMARK_SV)
	void testSV(size_t nv, uint32_t* off, uint32_t* ind) {
		uint32_t* components_map = (uint32_t*)memalign(64, nv * sizeof(uint32_t));

		{
			printf("BFS regular:\n");
			for (size_t i = 0; i < nv; i++) {
				components_map[i] = i;
			}
			bool changed = true;
			size_t iteration = 0;
			while (changed) {
				tic();
				changed = SVSeq(nv, components_map, off, ind);
				const double secs = toc() * 1.0e+9;
				if ((iteration++ < 3) || (!changed))
					printf("\tIteration %3zu: %lf\n", iteration, secs);
			}
		}

		{
			printf("BFS branchless:\n");
			for (size_t i = 0; i < nv; i++) {
				components_map[i] = i;
			}
			bool changed = true;
			size_t iteration = 0;
			while (changed) {
				tic();
				changed = SVBranchless(nv, components_map, off, ind);
				const double secs = toc() * 1.0e+9;
				if ((iteration++ < 3) || (!changed))
					printf("\tIteration %3zu: %lf\n", iteration, secs);
			}
		}

	#ifndef __MIC__
		{
			printf("BFS branchless Asm:\n");
			for (size_t i = 0; i < nv; i++) {
				components_map[i] = i;
			}
			bool changed = true;
			size_t iteration = 0;
			while (changed) {
				tic();
				changed = SVBranchlessAsm(nv, components_map, off, ind);
				const double secs = toc() * 1.0e+9;
				if ((iteration++ < 3) || (!changed))
					printf("\tIteration %3zu: %lf\n", iteration, secs);
			}
		}
	#endif

	#ifdef __SSE4_1__
		{
			printf("BFS branchless SSE4.1:\n");
			for (size_t i = 0; i < nv; i++) {
				components_map[i] = i;
			}
			bool changed = true;
			size_t iteration = 0;
			while (changed) {
				tic();
				changed = SVBranchlessSSE4_1(nv, components_map, off, ind);
				const double secs = toc() * 1.0e+9;
				if ((iteration++ < 3) || (!changed))
					printf("\tIteration %3zu: %lf\n", iteration, secs);
			}
		}
	#endif

	#ifdef __MIC__
		{
			printf("BFS branchless MIC:\n");
			for (size_t i = 0; i < nv; i++) {
				components_map[i] = i;
			}
			bool changed = true;
			size_t iteration = 0;
			while (changed) {
				tic();
				changed = SVBranchlessMIC(nv, components_map, off, ind);
				const double secs = toc() * 1.0e+9;
				if ((iteration++ < 3) || (!changed))
					printf("\tIteration %3zu: %lf\n", iteration, secs);
			}
		}
	#endif

		free(components_map);
	}
#endif
