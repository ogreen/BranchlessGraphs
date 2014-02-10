#include "main.h"
#include "timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <inttypes.h>

#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>

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

void testSV(const char* implementation_name, SVFunction sv_function, size_t nv, uint32_t* off, uint32_t* ind);
void testBFS(const char* implementation_name, BFSFunction bfs_function, uint32_t* off, uint32_t* ind);

#define LINE_SIZE 10000

static int perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags) {
	return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

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
        testBFS("BFS brachy", BFSSeq, off, ind);
        testBFS("BFS branchless (C)", BFSSeqBranchless, off, ind);
	#ifndef __MIC__
        testBFS("BFS branchless (asm)", BFSSeqBranchlessAsm, off, ind);
	#endif
	#ifdef __SSE4_1__
        testBFS("BFS bracnhless (SSE 4.1)", BFSSeqBranchlessSSE4_1, off, ind);
	#endif
	#ifdef __AVX2__
        testBFS("BFS bracnhless (AVX 2)", BFSSeqBranchlessAVX2, off, ind);
	#endif
	#ifdef __MIC__
        testBFS("BFS bracnhless (MIC)", BFSSeqBranchlessMIC, off, ind);
	#endif
#endif
#if defined(BENCHMARK_SV)
	testSV("SV regular", SVSeq, nv, off, ind);
	testSV("SV branchless (C)", SVBranchless, nv, off, ind);
	#ifndef __MIC__
	testSV("SV branchless (asm)", SVBranchlessAsm, nv, off, ind);
	#endif
	#ifdef __SSE4_1__
	testSV("SV branchless (SSE4.1)", SVBranchlessSSE4_1, nv, off, ind);
	#endif
	#ifdef __MIC__
	testSV("SV branchless (MIC)", SVBranchlessMIC, nv, off, ind);
	#endif
#endif
 	free(off);
	free(ind);
}

#if defined(BENCHMARK_BFS)
	void testBFS(const char* implementation_name, BFSFunction bfs_function, uint32_t* off, uint32_t* ind) {
		struct perf_event_attr perf_branches;
		struct perf_event_attr perf_mispredictions;
		struct perf_event_attr perf_instructions;

		memset(&perf_branches, 0, sizeof(struct perf_event_attr));
		perf_branches.type = PERF_TYPE_HARDWARE;
		perf_branches.size = sizeof(struct perf_event_attr);
		perf_branches.config = PERF_COUNT_HW_BRANCH_INSTRUCTIONS;
		perf_branches.disabled = 1;
		perf_branches.exclude_kernel = 1;
		perf_branches.exclude_hv = 1;

		memset(&perf_mispredictions, 0, sizeof(struct perf_event_attr));
		perf_mispredictions.type = PERF_TYPE_HARDWARE;
		perf_mispredictions.size = sizeof(struct perf_event_attr);
		perf_mispredictions.config = PERF_COUNT_HW_BRANCH_MISSES;
		perf_mispredictions.disabled = 1;
		perf_mispredictions.exclude_kernel = 1;
		perf_mispredictions.exclude_hv = 1;

		memset(&perf_instructions, 0, sizeof(struct perf_event_attr));
		perf_instructions.type = PERF_TYPE_HARDWARE;
		perf_instructions.size = sizeof(struct perf_event_attr);
		perf_instructions.config = PERF_COUNT_HW_INSTRUCTIONS;
		perf_instructions.disabled = 1;
		perf_instructions.exclude_kernel = 1;
		perf_instructions.exclude_hv = 1;

		int fd_branches = perf_event_open(&perf_branches, 0, -1, -1, 0);
		if (fd_branches == -1) {
			fprintf(stderr, "Error opening PERF_COUNT_HW_BRANCH_INSTRUCTIONS\n");
			exit(EXIT_FAILURE);
		}

		int fd_mispredictions = perf_event_open(&perf_mispredictions, 0, -1, -1, 0);
		if (fd_mispredictions == -1) {
			fprintf(stderr, "Error opening PERF_COUNT_HW_BRANCH_MISSES\n");
			exit(EXIT_FAILURE);
		}

		int fd_instructions = perf_event_open(&perf_instructions, 0, -1, -1, 0);
		if (fd_instructions == -1) {
			fprintf(stderr, "Error opening PERF_COUNT_HW_INSTRUCTIONS\n");
			exit(EXIT_FAILURE);
		}

		uint32_t* queue = (uint32_t*)memalign(64, nv * sizeof(uint32_t));
		uint32_t* level = (uint32_t*)memalign(64, nv * sizeof(uint32_t));
		
		ioctl(fd_branches, PERF_EVENT_IOC_RESET, 0);
		ioctl(fd_mispredictions, PERF_EVENT_IOC_RESET, 0);
		ioctl(fd_instructions, PERF_EVENT_IOC_RESET, 0);
		{
			/* Initialize level array */
			for (size_t i = 0; i < nv; i++) {
				level[i] = INT32_MAX;
			}
			tic();
			ioctl(fd_branches, PERF_EVENT_IOC_ENABLE, 0);
			ioctl(fd_mispredictions, PERF_EVENT_IOC_ENABLE, 0);
			ioctl(fd_instructions, PERF_EVENT_IOC_ENABLE, 0);
			
			/* Call the bfs implementation */
			bfs_function(off, ind, queue, level, 1);

			ioctl(fd_branches, PERF_EVENT_IOC_DISABLE, 0);
			ioctl(fd_mispredictions, PERF_EVENT_IOC_DISABLE, 0);
			ioctl(fd_instructions, PERF_EVENT_IOC_DISABLE, 0);
			const double nanoseconds = toc() * 1.0e+9;
			long long branches, mispredictions, instructions;
			read(fd_branches, &branches, sizeof(long long));
			read(fd_mispredictions, &mispredictions, sizeof(long long));
			read(fd_instructions, &instructions, sizeof(long long));
			const double branch_misprections = (double)mispredictions / (double)branches;
			printf("%30s\t%9.lf\t%5.3lf%%\t%lld\t%lld\t%lld\n", implementation_name, nanoseconds, branch_misprections * 100.0, mispredictions, branches, instructions);
		}

		close(fd_branches);
		close(fd_mispredictions);
		close(fd_instructions);
		free(queue);
		free(level);
	}
#endif

#if defined(BENCHMARK_SV)
	void testSV(const char* implementation_name, SVFunction sv_function, size_t nv, uint32_t* off, uint32_t* ind) {
		uint32_t* components_map = (uint32_t*)memalign(64, nv * sizeof(uint32_t));

		struct perf_event_attr perf_branches;
		struct perf_event_attr perf_mispredictions;
		struct perf_event_attr perf_instructions;

		memset(&perf_branches, 0, sizeof(struct perf_event_attr));
		perf_branches.type = PERF_TYPE_HARDWARE;
		perf_branches.size = sizeof(struct perf_event_attr);
		perf_branches.config = PERF_COUNT_HW_BRANCH_INSTRUCTIONS;
		perf_branches.disabled = 1;
		perf_branches.exclude_kernel = 1;
		perf_branches.exclude_hv = 1;

		memset(&perf_mispredictions, 0, sizeof(struct perf_event_attr));
		perf_mispredictions.type = PERF_TYPE_HARDWARE;
		perf_mispredictions.size = sizeof(struct perf_event_attr);
		perf_mispredictions.config = PERF_COUNT_HW_BRANCH_MISSES;
		perf_mispredictions.disabled = 1;
		perf_mispredictions.exclude_kernel = 1;
		perf_mispredictions.exclude_hv = 1;

		memset(&perf_instructions, 0, sizeof(struct perf_event_attr));
		perf_instructions.type = PERF_TYPE_HARDWARE;
		perf_instructions.size = sizeof(struct perf_event_attr);
		perf_instructions.config = PERF_COUNT_HW_INSTRUCTIONS;
		perf_instructions.disabled = 1;
		perf_instructions.exclude_kernel = 1;
		perf_instructions.exclude_hv = 1;

		int fd_branches = perf_event_open(&perf_branches, 0, -1, -1, 0);
		if (fd_branches == -1) {
			fprintf(stderr, "Error opening PERF_COUNT_HW_BRANCH_INSTRUCTIONS\n");
			exit(EXIT_FAILURE);
		}

		int fd_mispredictions = perf_event_open(&perf_mispredictions, 0, -1, -1, 0);
		if (fd_mispredictions == -1) {
			fprintf(stderr, "Error opening PERF_COUNT_HW_BRANCH_MISSES\n");
			exit(EXIT_FAILURE);
		}

		int fd_instructions = perf_event_open(&perf_instructions, 0, -1, -1, 0);
		if (fd_instructions == -1) {
			fprintf(stderr, "Error opening PERF_COUNT_HW_INSTRUCTIONS\n");
			exit(EXIT_FAILURE);
		}

		{
			printf("%s:\n", implementation_name);
			for (size_t i = 0; i < nv; i++) {
				components_map[i] = i;
			}
			bool changed = true;
			size_t iteration = 0;
			while (changed) {
				ioctl(fd_branches, PERF_EVENT_IOC_RESET, 0);
				ioctl(fd_mispredictions, PERF_EVENT_IOC_RESET, 0);
				ioctl(fd_instructions, PERF_EVENT_IOC_RESET, 0);
				tic();
				ioctl(fd_branches, PERF_EVENT_IOC_ENABLE, 0);
				ioctl(fd_mispredictions, PERF_EVENT_IOC_ENABLE, 0);
				ioctl(fd_instructions, PERF_EVENT_IOC_ENABLE, 0);

				changed = sv_function(nv, components_map, off, ind);

				ioctl(fd_branches, PERF_EVENT_IOC_DISABLE, 0);
				ioctl(fd_mispredictions, PERF_EVENT_IOC_DISABLE, 0);
				ioctl(fd_instructions, PERF_EVENT_IOC_DISABLE, 0);
				const double secs = toc() * 1.0e+9;
				long long branches, mispredictions, instructions;
				read(fd_branches, &branches, sizeof(long long));
				read(fd_mispredictions, &mispredictions, sizeof(long long));
				read(fd_instructions, &instructions, sizeof(long long));
				const double branch_misprections = (double)(mispredictions) / (double)(branches - nv);
				if ((iteration++ < 7) || (!changed))
					printf("\tIteration %3zu: %9.lf\t%5.3lf%%\t%11lld\t%11lld\t%11lld\n", iteration, secs, branch_misprections * 100.0, mispredictions, branches, instructions);
			}
		}

		close(fd_branches);
		close(fd_mispredictions);
		close(fd_instructions);
		free(components_map);
	}
#endif
