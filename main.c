#include "main.h"
#include "timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <inttypes.h>
#include <assert.h>

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

void Benchmark_BFS_TopDown(const char* implementation_name, BFS_TopDown_Function bfs_function, uint32_t* off, uint32_t* ind);
void Benchmark_BFS_BottomUp(const char* implementation_name, BFS_BottomUp_Function bfs_function, uint32_t* off, uint32_t* ind);
void Benchmark_ConnectedComponents_SV(const char* implementation_name, ConnectedComponents_SV_Function sv_function, size_t nv, uint32_t* off, uint32_t* ind);

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
		{
			uint32_t* queue = (uint32_t*)memalign(64, nv * sizeof(uint32_t));
			uint32_t* level = (uint32_t*)memalign(64, nv * sizeof(uint32_t));
			uint32_t* edgesTraversed = (uint32_t*)memalign(64, nv * sizeof(uint32_t));
			uint32_t* queueStartPosition = (uint32_t*)memalign(64, nv * sizeof(uint32_t));
			for (size_t i = 0; i < nv; i++) {
				level[i] = INT32_MAX;
			}

			BFS_TopDown_Branchy_LevelInformation(off, ind, queue, level, 1, edgesTraversed, queueStartPosition);

			free(queueStartPosition);
			free(edgesTraversed);
			free(level);
			free(queue);
		}

		Benchmark_BFS_TopDown("BFS/TD brachy", BFS_TopDown_Branchy, off, ind);
		Benchmark_BFS_TopDown("BFS/TD branchless (C)", BFS_TopDown_Branchless, off, ind);
		#if defined(__x86_64__) && !defined(__MIC__)
		Benchmark_BFS_TopDown("BFS/TD branchless (CMOV)", BFS_TopDown_Branchless_CMOV, off, ind);
		#endif
		#ifdef __SSE4_1__
		Benchmark_BFS_TopDown("BFS/TD bracnhless (SSE 4.1)", BFS_TopDown_Branchless_SSE4_1, off, ind);
		#endif
		#ifdef __AVX2__
		//~ Benchmark_BFS_TopDown("BFS/TD bracnhless (AVX 2)", BFS_TopDown_Branchless_AVX2, off, ind);
		#endif
		#ifdef __MIC__
		//~ Benchmark_BFS_TopDown("BFS/TD bracnhless (MIC)", BFS_TopDown_Branchless_MIC, off, ind);
		#endif
		//~ Benchmark_BFS_BottomUp("BFS/BU brachy", BFS_BottomUp_Branchy, off, ind);
		//~ Benchmark_BFS_BottomUp("BFS/BU branchless (C)", BFS_BottomUp_Branchless, off, ind);
		//~ Benchmark_BFS_BottomUp("BFS/BU branchless (CMOV)", BFS_BottomUp_Branchless_CMOV, off, ind);

	#endif

#if defined(BENCHMARK_SV)
	Benchmark_ConnectedComponents_SV("SV regular", ConnectedComponents_SV_Branchy, nv, off, ind);
	Benchmark_ConnectedComponents_SV("SV branchless (C)", ConnectedComponents_SV_Branchless, nv, off, ind);
	#if defined(__x86_64__) && !defined(__MIC__)
	Benchmark_ConnectedComponents_SV("SV branchless (asm)", ConnectedComponents_SV_Branchless_CMOV, nv, off, ind);
	#endif
	#ifdef __SSE4_1__
	Benchmark_ConnectedComponents_SV("SV branchless (SSE4.1)", ConnectedComponents_SV_Branchless_SSE4_1, nv, off, ind);
	#endif
	#ifdef __MIC__
	Benchmark_ConnectedComponents_SV("SV branchless (MIC)", ConnectedComponents_SV_Branchless_MIC, nv, off, ind);
	#endif
#endif
 	free(off);
	free(ind);
}

#if defined(BENCHMARK_BFS)
	void Benchmark_BFS_TopDown(const char* implementation_name, BFS_TopDown_Function bfs_function, uint32_t* off, uint32_t* ind) {
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

		uint32_t* queue = (uint32_t*)memalign(64, (nv + 3) * sizeof(uint32_t));
		uint32_t* level = (uint32_t*)memalign(64, nv * sizeof(uint32_t));
		/* Initialize level array */
		for (size_t i = 0; i < nv; i++) {
			level[i] = INT32_MAX;
		}

		uint64_t* branches = (uint64_t*)memalign(64, nv * sizeof(uint64_t));
		uint64_t* mispredictions = (uint64_t*)memalign(64, nv * sizeof(uint64_t));
		uint64_t* instructions = (uint64_t*)memalign(64, nv * sizeof(uint64_t));
		double* seconds = (double*)memalign(64, nv * sizeof(double));

		const uint32_t rootVertex = 1;
		uint32_t currentLevel = 0;
		level[rootVertex] = currentLevel++;
		uint32_t* queuePosition = queue;
		queue[0] = rootVertex;

		uint32_t outputVerteces = 1;
		do {
			ioctl(fd_branches, PERF_EVENT_IOC_RESET, 0);
			ioctl(fd_mispredictions, PERF_EVENT_IOC_RESET, 0);
			ioctl(fd_instructions, PERF_EVENT_IOC_RESET, 0);

			tic();
			ioctl(fd_branches, PERF_EVENT_IOC_ENABLE, 0);
			ioctl(fd_mispredictions, PERF_EVENT_IOC_ENABLE, 0);
			ioctl(fd_instructions, PERF_EVENT_IOC_ENABLE, 0);

			const uint32_t inputVerteces = outputVerteces;
			outputVerteces = bfs_function(off, ind, queuePosition, inputVerteces, queuePosition + inputVerteces, level, currentLevel);
			queuePosition += inputVerteces;

			ioctl(fd_branches, PERF_EVENT_IOC_DISABLE, 0);
			ioctl(fd_mispredictions, PERF_EVENT_IOC_DISABLE, 0);
			ioctl(fd_instructions, PERF_EVENT_IOC_DISABLE, 0);
			seconds[currentLevel] = toc();
			read(fd_branches, &branches[currentLevel], sizeof(uint64_t));
			read(fd_mispredictions, &mispredictions[currentLevel], sizeof(uint64_t));
			read(fd_instructions, &instructions[currentLevel], sizeof(uint64_t));
			currentLevel += 1;
		} while (outputVerteces != 0);

		for (uint32_t level = 0; level < currentLevel; level++) {
			printf("%s\t%"PRIu32"\t%.1lf\t%"PRIu64"\t%"PRIu64"\t%"PRIu64"\n", implementation_name, level, seconds[level] *1.0e+9, mispredictions[level], branches[level], instructions[level]);
		}

		close(fd_branches);
		close(fd_mispredictions);
		close(fd_instructions);
		free(queue);
		free(level);
		free(branches);
		free(mispredictions);
		free(instructions);
		free(seconds);
	}

	void Benchmark_BFS_BottomUp(const char* implementation_name, BFS_BottomUp_Function bfs_function, uint32_t* off, uint32_t* ind) {
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

		uint32_t* levels = (uint32_t*)memalign(64, nv * sizeof(uint32_t));
		uint32_t* bitmap = (uint32_t*)memalign(64, nv * sizeof(uint32_t));
		/* Initialize level array */
		for (size_t i = 0; i < nv; i++) {
			levels[i] = INT32_MAX;
			bitmap[i] = 0;
		}

		uint64_t* branches = (uint64_t*)memalign(64, nv * sizeof(uint64_t));
		uint64_t* mispredictions = (uint64_t*)memalign(64, nv * sizeof(uint64_t));
		uint64_t* instructions = (uint64_t*)memalign(64, nv * sizeof(uint64_t));
		double* seconds = (double*)memalign(64, nv * sizeof(double));

		uint32_t currentLevel = 0;
		bool changed;

		const uint32_t rootVertex = 1;
		levels[rootVertex] = currentLevel++;
		bitmap[rootVertex / 32] = 1 << (rootVertex % 32);
		do {
			ioctl(fd_branches, PERF_EVENT_IOC_RESET, 0);
			ioctl(fd_mispredictions, PERF_EVENT_IOC_RESET, 0);
			ioctl(fd_instructions, PERF_EVENT_IOC_RESET, 0);

			tic();
			ioctl(fd_branches, PERF_EVENT_IOC_ENABLE, 0);
			ioctl(fd_mispredictions, PERF_EVENT_IOC_ENABLE, 0);
			ioctl(fd_instructions, PERF_EVENT_IOC_ENABLE, 0);

			/* Call the bfs implementation */
			changed = bfs_function(off, ind, bitmap, levels, nv, currentLevel);

			ioctl(fd_branches, PERF_EVENT_IOC_DISABLE, 0);
			ioctl(fd_mispredictions, PERF_EVENT_IOC_DISABLE, 0);
			ioctl(fd_instructions, PERF_EVENT_IOC_DISABLE, 0);
			seconds[currentLevel] = toc();
			read(fd_branches, &branches[currentLevel], sizeof(long long));
			read(fd_mispredictions, &mispredictions[currentLevel], sizeof(long long));
			read(fd_instructions, &instructions[currentLevel], sizeof(long long));
			currentLevel += 1;
		} while (changed);

		for (uint32_t level = 0; level < currentLevel; level++) {
			printf("%s\t%"PRIu32"\t%.1lf\t%"PRIu64"\t%"PRIu64"\t%"PRIu64"\n", implementation_name, level, seconds[level] *1.0e+9, mispredictions[level], branches[level], instructions[level]);
		}

		close(fd_branches);
		close(fd_mispredictions);
		close(fd_instructions);
		free(levels);
		free(bitmap);
		free(branches);
		free(mispredictions);
		free(instructions);
		free(seconds);
	}
#endif

#if defined(BENCHMARK_SV)
	void Benchmark_ConnectedComponents_SV(const char* implementation_name, ConnectedComponents_SV_Function sv_function, size_t nv, uint32_t* off, uint32_t* ind) {
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
