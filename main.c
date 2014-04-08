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

void Benchmark_BFS_TopDown(const char* implementation_name, BFS_TopDown_Function bfs_function, uint32_t numVerteces, uint32_t* off, uint32_t* ind, uint32_t* edgesTraversed);
void Benchmark_BFS_BottomUp(const char* implementation_name, BFS_BottomUp_Function bfs_function, uint32_t numVerteces, uint32_t* off, uint32_t* ind);
void Benchmark_ConnectedComponents_SV(const char* implementation_name, ConnectedComponents_SV_Function sv_function, size_t numVerteces, size_t numEdges, uint32_t* off, uint32_t* ind);

#define LINE_SIZE 10000

static int perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags) {
	return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

void readGraphDIMACS(char* filePath, uint32_t** prmoff, uint32_t** prmind, uint32_t* prmnv, uint32_t* prmne){
	FILE *fp = fopen (filePath, "r");
	int32_t nv,ne;

	char* line=NULL;
	
	// Read data from file
	int32_t temp,lineRead;
	size_t bytesRead=0;
	getline (&line, &bytesRead, fp);	
	
//	fgets (line, bytesRead, fp);
	sscanf (line, "%d %d", &nv, &ne);
//	printf ( "%ld %ld\n", nv, ne);		
		
	free(line);
	int32_t * off = (int32_t *) malloc ((nv + 2) * sizeof (int32_t));
	int32_t * ind = (int32_t *) malloc ((ne * 2) * sizeof (int32_t));
	off[0] = 0;
	off[1] = 0;
	int32_t counter = 0;
	int32_t u;
	line=NULL;
	bytesRead=0;

//	  for (u = 1; fgets (line, &bytesRead, fp); u++)
	for (u = 1; (temp=getline (&line, &bytesRead, fp))!=-1; u++)
	{	
//		printf("%s",line);	
/*		bytesRead=0;	
		free(line);	
		if (u>10) 
			break;

		continue;
*/		
		uint32_t neigh = 0;
		uint32_t v = 0;
		char *ptr = line;
		int read = 0;
		char tempStr[1000];
		lineRead=0;
		while (lineRead<bytesRead && (read=sscanf (ptr, "%s", tempStr)) > 0)
		{
			v=atoi(tempStr);
			read=strlen(tempStr);
			ptr += read+1;
			lineRead=read+1;
			neigh++;
			ind[counter++] = v;
		}
		off[u + 1] = off[u] + neigh;
		free(line);	  
		bytesRead=0;
	}


	  fclose (fp);


	   nv++;
	   ne*=2;
	*prmnv=nv;
	*prmne=ne;
	*prmind=ind;
	*prmoff=off;
}



int main (const int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: bfs <graph-name>\n");
		exit(EXIT_FAILURE);
	}

    uint32_t nv, ne, naction;
    uint32_t* off;
    uint32_t* ind;
	readGraphDIMACS(argv[1], &off, &ind, &nv, &ne);
	

	#if defined(BENCHMARK_BFS)	
		uint32_t* edgesTraversed = (uint32_t*)memalign(64, nv * sizeof(uint32_t));
		memset(edgesTraversed, 0, nv * sizeof(uint32_t));
		{
			uint32_t* queue = (uint32_t*)memalign(64, nv * sizeof(uint32_t));
			uint32_t* level = (uint32_t*)memalign(64, nv * sizeof(uint32_t));
			uint32_t* queueStartPosition = (uint32_t*)memalign(64, nv * sizeof(uint32_t));
			for (size_t i = 0; i < nv; i++) {
				level[i] = INT32_MAX;
			}

			BFS_TopDown_Branchy_LevelInformation(off, ind, queue, level, 1, edgesTraversed, queueStartPosition);

			free(queueStartPosition);
			free(level);
			free(queue);
		}

		Benchmark_BFS_TopDown("BFS/TD brachy", BFS_TopDown_Branchy, nv, off, ind, edgesTraversed);
		//~ Benchmark_BFS_TopDown("BFS/TD brachy+reordered", BFS_TopDown_BranchyPlus, off, ind, edgesTraversed);
		Benchmark_BFS_TopDown("BFS/TD branchless (C)", BFS_TopDown_Branchless, nv, off, ind, edgesTraversed);
        #ifdef __arm__
		Benchmark_BFS_TopDown("BFS/TD brachy (Peach-Py)", _BFS_TopDown_Branchy_CortexA15, nv, off, ind, edgesTraversed);
		Benchmark_BFS_TopDown("BFS/TD branchless (Peach-Py)", _BFS_TopDown_Branchless_CortexA15, nv, off, ind, edgesTraversed);
        #elif defined(__x86_64__)
		Benchmark_BFS_TopDown("BFS/TD brachy (Peach-Py)", BFS_TopDown_Branchy_PeachPy, nv, off, ind, edgesTraversed);
		Benchmark_BFS_TopDown("BFS/TD brachless (Peach-Py)", BFS_TopDown_Branchless_PeachPy, nv, off, ind, edgesTraversed);
        #else
		//~ Benchmark_BFS_TopDown("BFS/TD brachy (Peach-Py)", _BFS_TopDown_Branchy_Unknown, nv, off, ind, edgesTraversed);
        #endif
		#if defined(__x86_64__) && !defined(__MIC__)
		//~ Benchmark_BFS_TopDown("BFS/TD branchless (CMOV)", BFS_TopDown_Branchless_CMOV, nv, off, ind, edgesTraversed);
		//~ Benchmark_BFS_TopDown("BFS/TD branchless+reordered (CMOV)", BFS_TopDown_Branchless_CMOVPlus, nv, off, ind, edgesTraversed);
		#endif
		#ifdef __SSE4_1__
		//~ Benchmark_BFS_TopDown("BFS/TD bracnhless (SSE 4.1)", BFS_TopDown_Branchless_SSE4_1, nv, off, ind, edgesTraversed);
		#endif
		#ifdef __AVX2__
		//~ Benchmark_BFS_TopDown("BFS/TD bracnhless (AVX 2)", BFS_TopDown_Branchless_AVX2, nv, off, ind, edgesTraversed);
		#endif
		#ifdef __MIC__
		//~ Benchmark_BFS_TopDown("BFS/TD bracnhless (MIC)", BFS_TopDown_Branchless_MIC, nv, off, ind, edgesTraversed);
		#endif
		Benchmark_BFS_BottomUp("BFS/BU brachy", BFS_BottomUp_Branchy, nv, off, ind);
		//~ Benchmark_BFS_BottomUp("BFS/BU branchless (C)", BFS_BottomUp_Branchless, nv, off, ind);
		//~ Benchmark_BFS_BottomUp("BFS/BU branchless (CMOV)", BFS_BottomUp_Branchless_CMOV, nv, off, ind);

		free(edgesTraversed);
	#endif

#if defined(BENCHMARK_SV)
    Benchmark_ConnectedComponents_SV("SV branchy", ConnectedComponents_SV_Branchy, nv, ne, off, ind);
	Benchmark_ConnectedComponents_SV("SV branchless (C)", ConnectedComponents_SV_Branchless, nv, ne, off, ind);
	Benchmark_ConnectedComponents_SV("SV branchy (Peach-Py)", ConnectedComponents_SV_Branchy_PeachPy, nv, ne, off, ind);
	Benchmark_ConnectedComponents_SV("SV branchless (Peach-Py)", ConnectedComponents_SV_Branchless_PeachPy, nv, ne, off, ind);
	#if defined(__x86_64__) && !defined(__MIC__)
	Benchmark_ConnectedComponents_SV("SV branchless (asm)", ConnectedComponents_SV_Branchless_CMOV, nv, ne, off, ind);
	#endif
	#ifdef __SSE4_1__
	Benchmark_ConnectedComponents_SV("SV branchless (SSE4.1)", ConnectedComponents_SV_Branchless_SSE4_1, nv, ne, off, ind);
	#endif
	#ifdef __MIC__
	Benchmark_ConnectedComponents_SV("SV branchless (MIC)", ConnectedComponents_SV_Branchless_MIC, nv, ne, off, ind);
	#endif
#endif
 	free(off);
	free(ind);
}

#if defined(BENCHMARK_BFS)
	void Benchmark_BFS_TopDown(const char* implementation_name, BFS_TopDown_Function bfs_function, uint32_t numVerteces, uint32_t* off, uint32_t* ind, uint32_t* edgesTraversed) {
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

		uint32_t* queue = (uint32_t*)memalign(64, (numVerteces + 3) * sizeof(uint32_t));
		uint32_t* level = (uint32_t*)memalign(64, numVerteces * sizeof(uint32_t));
		/* Initialize level array */
		for (size_t i = 0; i < numVerteces; i++) {
			level[i] = INT32_MAX;
		}

		uint64_t* branches = (uint64_t*)memalign(64, numVerteces * sizeof(uint64_t));
		uint64_t* mispredictions = (uint64_t*)memalign(64, numVerteces * sizeof(uint64_t));
		uint64_t* instructions = (uint64_t*)memalign(64, numVerteces * sizeof(uint64_t));
		uint32_t* vertices = (uint32_t*)memalign(64, numVerteces * sizeof(uint32_t));
		double* seconds = (double*)memalign(64, numVerteces * sizeof(double));

		const uint32_t rootVertex = 1;
		uint32_t currentLevel = 0;
		level[rootVertex] = currentLevel++;
		uint32_t* queuePosition = queue;
		queue[0] = rootVertex;

		uint32_t outputVerteces = 1;
		do {
			const uint32_t inputVerteces = outputVerteces;
			vertices[currentLevel-1] = inputVerteces;

			assert(ioctl(fd_branches, PERF_EVENT_IOC_RESET, 0) == 0);
			assert(ioctl(fd_mispredictions, PERF_EVENT_IOC_RESET, 0) == 0);
			assert(ioctl(fd_instructions, PERF_EVENT_IOC_RESET, 0) == 0);

			assert(ioctl(fd_branches, PERF_EVENT_IOC_ENABLE, 0) == 0);
			assert(ioctl(fd_mispredictions, PERF_EVENT_IOC_ENABLE, 0) == 0);
			assert(ioctl(fd_instructions, PERF_EVENT_IOC_ENABLE, 0) == 0);
			tic();

			outputVerteces = bfs_function(off, ind, queuePosition, inputVerteces, queuePosition + inputVerteces, level, currentLevel);
			queuePosition += inputVerteces;

			seconds[currentLevel-1] = toc();
			assert(ioctl(fd_branches, PERF_EVENT_IOC_DISABLE, 0) == 0);
			assert(ioctl(fd_mispredictions, PERF_EVENT_IOC_DISABLE, 0) == 0);
			assert(ioctl(fd_instructions, PERF_EVENT_IOC_DISABLE, 0) == 0);
			assert(read(fd_branches, &branches[currentLevel-1], sizeof(uint64_t)) == sizeof(uint64_t));
			assert(read(fd_mispredictions, &mispredictions[currentLevel-1], sizeof(uint64_t)) == sizeof(uint64_t));
			assert(read(fd_instructions, &instructions[currentLevel-1], sizeof(uint64_t)) == sizeof(uint64_t));
			currentLevel += 1;
		} while (outputVerteces != 0);
		const uint32_t levelCount = currentLevel - 1;

		for (uint32_t level = 0; level < levelCount; level++) {
			printf("%s\t%"PRIu32"\t%.10lf\t%"PRIu64"\t%"PRIu64"\t%"PRIu64"\t%"PRIu32"\t%"PRIu32"\n", implementation_name, level, seconds[level], mispredictions[level], branches[level], instructions[level], vertices[level], edgesTraversed[level]);
		}

		close(fd_branches);
		close(fd_mispredictions);
		close(fd_instructions);
		free(queue);
		free(level);
		free(branches);
		free(mispredictions);
		free(instructions);
		free(vertices);
		free(seconds);
	}

	void Benchmark_BFS_BottomUp(const char* implementation_name, BFS_BottomUp_Function bfs_function, uint32_t numVerteces, uint32_t* off, uint32_t* ind) {
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

		uint32_t* levels = (uint32_t*)memalign(64, numVerteces * sizeof(uint32_t));
		uint32_t* bitmap = (uint32_t*)memalign(64, numVerteces * sizeof(uint32_t));
		/* Initialize level array */
		for (size_t i = 0; i < numVerteces; i++) {
			levels[i] = INT32_MAX;
			bitmap[i] = 0;
		}

		uint64_t* branches = (uint64_t*)memalign(64, numVerteces * sizeof(uint64_t));
		uint64_t* mispredictions = (uint64_t*)memalign(64, numVerteces * sizeof(uint64_t));
		uint64_t* instructions = (uint64_t*)memalign(64, numVerteces * sizeof(uint64_t));
		double* seconds = (double*)memalign(64, numVerteces * sizeof(double));

		uint32_t currentLevel = 0;
		bool changed;

		const uint32_t rootVertex = 1;
		levels[rootVertex] = currentLevel;
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
			changed = bfs_function(off, ind, bitmap, levels, numVerteces, currentLevel);

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
			printf("%s\t%"PRIu32"\t%.10lf\t%"PRIu64"\t%"PRIu64"\t%"PRIu64" \t1 \t1\n", implementation_name, level, seconds[level], mispredictions[level], branches[level], instructions[level]);
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
	void Benchmark_ConnectedComponents_SV(const char* implementation_name, ConnectedComponents_SV_Function sv_function, size_t numVertices, size_t numEdges, uint32_t* off, uint32_t* ind) {
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

		uint32_t* components_map = (uint32_t*)memalign(64, numVertices * sizeof(uint32_t));
		uint64_t* branches = (uint64_t*)memalign(64, numVertices * sizeof(uint64_t));
		uint64_t* mispredictions = (uint64_t*)memalign(64, numVertices * sizeof(uint64_t));
		uint64_t* instructions = (uint64_t*)memalign(64, numVertices * sizeof(uint64_t));
		double* seconds = (double*)memalign(64, numVertices * sizeof(double));

		for (size_t i = 0; i < numVertices; i++) {
			components_map[i] = i;
		}

		bool changed;
		size_t iteration = 0;
		do {
			ioctl(fd_branches, PERF_EVENT_IOC_RESET, 0);
			ioctl(fd_mispredictions, PERF_EVENT_IOC_RESET, 0);
			ioctl(fd_instructions, PERF_EVENT_IOC_RESET, 0);
			tic();
			ioctl(fd_branches, PERF_EVENT_IOC_ENABLE, 0);
			ioctl(fd_mispredictions, PERF_EVENT_IOC_ENABLE, 0);
			ioctl(fd_instructions, PERF_EVENT_IOC_ENABLE, 0);

			changed = sv_function(numVertices, components_map, off, ind);

			ioctl(fd_branches, PERF_EVENT_IOC_DISABLE, 0);
			ioctl(fd_mispredictions, PERF_EVENT_IOC_DISABLE, 0);
			ioctl(fd_instructions, PERF_EVENT_IOC_DISABLE, 0);
			seconds[iteration] = toc();
			read(fd_branches, &branches[iteration], sizeof(long long));
			read(fd_mispredictions, &mispredictions[iteration], sizeof(long long));
			read(fd_instructions, &instructions[iteration], sizeof(long long));
			iteration += 1;
		} while (changed);

		for (uint32_t i = 0; i < iteration; i++) {
			printf("%s\t%"PRIu32"\t%.10lf\t%"PRIu64"\t%"PRIu64"\t%"PRIu64"\t%zu\t%zu\n", implementation_name, i, seconds[i], mispredictions[i], branches[i], instructions[i], numVertices, numEdges);
		}

		close(fd_branches);
		close(fd_mispredictions);
		close(fd_instructions);
		free(components_map);
		free(branches);
		free(mispredictions);
		free(instructions);
		free(seconds);
	}
#endif
