#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <inttypes.h>
#include <assert.h>
#include <time.h>
#include <math.h>


#ifdef __x86_64__
	#include <x86intrin.h>
#endif

#include "main.h"

#if defined(BENCHMARK_BFS)
 void Benchmark_BFS_TopDown_Trace(const char* algorithm_name, const char* implementation_name, const struct PerformanceCounter performanceCounters[], size_t performanceCounterCount, BFS_TopDown_Function bfs_function, uint32_t numVertices, uint32_t* off, uint32_t* ind, uint32_t* edgesTraversed, 
 		uint32_t* queueStartPosition,uint32_t* prelevel,uint32_t*prequeue) {
  struct perf_event_attr perf_counter;

  uint32_t* queue = (uint32_t*)memalign(64, numVertices * sizeof(uint32_t));
  uint32_t* level = (uint32_t*)memalign(64, numVertices * sizeof(uint32_t));
  uint64_t* perf_events = (uint64_t*)malloc(numVertices * sizeof(uint64_t));
  uint32_t* vertices = (uint32_t*)malloc(numVertices * sizeof(uint32_t));

 
	uint32_t levelCount = 0;


  levelCount = 0;
  for (size_t performanceCounterIndex = 0; performanceCounterIndex < performanceCounterCount; performanceCounterIndex++) {
	if (!performanceCounters[performanceCounterIndex].supported)
	  continue;
	int perf_counter_fd = -1;
	if (performanceCounters[performanceCounterIndex].type != PERF_TYPE_TIME) {
	  memset(&perf_counter, 0, sizeof(struct perf_event_attr));
	  perf_counter.type = performanceCounters[performanceCounterIndex].type;
	  perf_counter.size = sizeof(struct perf_event_attr);
	  perf_counter.config = performanceCounters[performanceCounterIndex].subtype;
	  perf_counter.disabled = 1;
	  perf_counter.exclude_kernel = 1;
	  perf_counter.exclude_hv = 1;

	  perf_counter_fd = perf_event_open(&perf_counter, 0, -1, -1, 0);
	  if (perf_counter_fd == -1) {
		fprintf(stderr, "Error opening counter %s\n", performanceCounters[performanceCounterIndex].name);
		exit(EXIT_FAILURE);
	  }
	}

	/* Initialize level array */
	for (size_t i = 0; i < numVertices; i++) {
//	  level[i] = INT32_MAX;
//	  queue[i] = 0;
	}

	const uint32_t rootVertex = 1;
	uint32_t currentLevel = 0;
	level[rootVertex] = currentLevel++;
	uint32_t* queuePosition = prequeue;
	queue[0] = rootVertex;

	uint32_t outputVertices = 1;
	do {
	  // const uint32_t inputVertices = outputVertices;
		const uint32_t inputVertices = queueStartPosition[currentLevel]-queueStartPosition[currentLevel-1];
	  if (levelCount == 0) {
		vertices[currentLevel-1] = inputVertices;
	  }

	  struct timespec startTime;
	  if (performanceCounters[performanceCounterIndex].type == PERF_TYPE_TIME) {
		assert(clock_gettime(CLOCK_MONOTONIC, &startTime) == 0);
	  } else {
		assert(ioctl(perf_counter_fd, PERF_EVENT_IOC_RESET, 0) == 0);
		assert(ioctl(perf_counter_fd, PERF_EVENT_IOC_ENABLE, 0) == 0);
	  }

	  // outputVertices = bfs_function(off, ind, queuePosition, inputVertices, queuePosition + inputVertices, level, currentLevel);
	  // queuePosition += inputVertices;


		outputVertices = bfs_function(off, ind, queuePosition, inputVertices, queuePosition + inputVertices, prelevel, currentLevel);
		queuePosition += inputVertices;


	  if (performanceCounters[performanceCounterIndex].type == PERF_TYPE_TIME) {
		struct timespec endTime;
		assert(clock_gettime(CLOCK_MONOTONIC, &endTime) == 0);
		perf_events[levelCount * performanceCounterIndex + (currentLevel-1)] =
		  (1000000000ll * endTime.tv_sec + endTime.tv_nsec) - 
		  (1000000000ll * startTime.tv_sec + startTime.tv_nsec);
	  } else {
		assert(ioctl(perf_counter_fd, PERF_EVENT_IOC_DISABLE, 0) == 0);
		assert(read(perf_counter_fd, &perf_events[levelCount * performanceCounterIndex + (currentLevel-1)], sizeof(uint64_t)) == sizeof(uint64_t));
	  }
	  currentLevel += 1;
	} while (queueStartPosition[currentLevel] != INT32_MAX);	  
	// } while (outputVertices != 0);
	if (levelCount == 0) {
	  levelCount = currentLevel - 1;
	  perf_events = realloc(perf_events, numVertices * sizeof(uint64_t) * levelCount);
	}
	close(perf_counter_fd);
  }
  for (uint32_t level = 0; level < levelCount; level++) {
	printf("%s\t%s\t%"PRIu32, algorithm_name, implementation_name, level);
	for (size_t performanceCounterIndex = 0; performanceCounterIndex < performanceCounterCount; performanceCounterIndex++) {
	  if (!performanceCounters[performanceCounterIndex].supported)
		continue;
	  printf("\t%"PRIu64, perf_events[levelCount * performanceCounterIndex + level]);
	}
	printf("\t%"PRIu32"\t%"PRIu32"\n", vertices[level], edgesTraversed[level]);
  }
  free(queue);
  free(level);
  free(perf_events);
  free(vertices);
}
 

void Benchmark_BFS_TopDown(const char* algorithm_name, const char* implementation_name, const struct PerformanceCounter performanceCounters[], size_t performanceCounterCount, BFS_TopDown_Function bfs_function, uint32_t numVertices, uint32_t* off, uint32_t* ind, uint32_t* edgesTraversed) {
  struct perf_event_attr perf_counter;

  uint32_t* queue = (uint32_t*)memalign(64, numVertices * sizeof(uint32_t));
  uint32_t* level = (uint32_t*)memalign(64, numVertices * sizeof(uint32_t));
  uint64_t* perf_events = (uint64_t*)malloc(numVertices * sizeof(uint64_t));
  uint32_t* vertices = (uint32_t*)malloc(numVertices * sizeof(uint32_t));

  uint32_t levelCount = 0;
  for (size_t performanceCounterIndex = 0; performanceCounterIndex < performanceCounterCount; performanceCounterIndex++) {
	if (!performanceCounters[performanceCounterIndex].supported)
	  continue;
	int perf_counter_fd = -1;
	if (performanceCounters[performanceCounterIndex].type != PERF_TYPE_TIME) {
	  memset(&perf_counter, 0, sizeof(struct perf_event_attr));
	  perf_counter.type = performanceCounters[performanceCounterIndex].type;
	  perf_counter.size = sizeof(struct perf_event_attr);
	  perf_counter.config = performanceCounters[performanceCounterIndex].subtype;
	  perf_counter.disabled = 1;
	  perf_counter.exclude_kernel = 1;
	  perf_counter.exclude_hv = 1;

	  perf_counter_fd = perf_event_open(&perf_counter, 0, -1, -1, 0);
	  if (perf_counter_fd == -1) {
		fprintf(stderr, "Error opening counter %s\n", performanceCounters[performanceCounterIndex].name);
		exit(EXIT_FAILURE);
	  }
	}

	/* Initialize level array */
	for (size_t i = 0; i < numVertices; i++) {
	  level[i] = INT32_MAX;
	  queue[i] = 0;
	}

	const uint32_t rootVertex = 1;
	uint32_t currentLevel = 0;
	level[rootVertex] = currentLevel++;
	uint32_t* queuePosition = queue;
	queue[0] = rootVertex;

	uint32_t outputVertices = 1;
	do {
	  const uint32_t inputVertices = outputVertices;
	  if (levelCount == 0) {
		vertices[currentLevel-1] = inputVertices;
	  }

	  struct timespec startTime;
	  if (performanceCounters[performanceCounterIndex].type == PERF_TYPE_TIME) {
		assert(clock_gettime(CLOCK_MONOTONIC, &startTime) == 0);
	  } else {
		assert(ioctl(perf_counter_fd, PERF_EVENT_IOC_RESET, 0) == 0);
		assert(ioctl(perf_counter_fd, PERF_EVENT_IOC_ENABLE, 0) == 0);
	  }

	  outputVertices = bfs_function(off, ind, queuePosition, inputVertices, queuePosition + inputVertices, level, currentLevel);
	  queuePosition += inputVertices;

	  if (performanceCounters[performanceCounterIndex].type == PERF_TYPE_TIME) {
		struct timespec endTime;
		assert(clock_gettime(CLOCK_MONOTONIC, &endTime) == 0);
		perf_events[levelCount * performanceCounterIndex + (currentLevel-1)] =
		  (1000000000ll * endTime.tv_sec + endTime.tv_nsec) - 
		  (1000000000ll * startTime.tv_sec + startTime.tv_nsec);
	  } else {
		assert(ioctl(perf_counter_fd, PERF_EVENT_IOC_DISABLE, 0) == 0);
		assert(read(perf_counter_fd, &perf_events[levelCount * performanceCounterIndex + (currentLevel-1)], sizeof(uint64_t)) == sizeof(uint64_t));
	  }
	  currentLevel += 1;
	} while (outputVertices != 0);
	if (levelCount == 0) {
	  levelCount = currentLevel - 1;
	  perf_events = realloc(perf_events, numVertices * sizeof(uint64_t) * levelCount);
	}
	close(perf_counter_fd);
  }
  for (uint32_t level = 0; level < levelCount; level++) {
	printf("%s\t%s\t%"PRIu32, algorithm_name, implementation_name, level);
	for (size_t performanceCounterIndex = 0; performanceCounterIndex < performanceCounterCount; performanceCounterIndex++) {
	  if (!performanceCounters[performanceCounterIndex].supported)
		continue;
	  printf("\t%"PRIu64, perf_events[levelCount * performanceCounterIndex + level]);
	}
	printf("\t%"PRIu32"\t%"PRIu32"\n", vertices[level], edgesTraversed[level]);
  }
  free(queue);
  free(level);
  free(perf_events);
  free(vertices);
}

void Benchmark_BFS_BottomUp(const char* algorithm_name, const char* implementation_name, BFS_BottomUp_Function bfs_function, uint32_t numVertices, uint32_t* off, uint32_t* ind) {
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

  uint32_t* levels = (uint32_t*)memalign(64, numVertices * sizeof(uint32_t));
  uint32_t* bitmap = (uint32_t*)memalign(64, numVertices * sizeof(uint32_t));
  /* Initialize level array */
  for (size_t i = 0; i < numVertices; i++) {
	levels[i] = INT32_MAX;
	bitmap[i] = 0;
  }

  uint64_t* branches = (uint64_t*)memalign(64, numVertices * sizeof(uint64_t));
  uint64_t* mispredictions = (uint64_t*)memalign(64, numVertices * sizeof(uint64_t));
  uint64_t* instructions = (uint64_t*)memalign(64, numVertices * sizeof(uint64_t));
  double* seconds = (double*)memalign(64, numVertices * sizeof(double));

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
	changed = bfs_function(off, ind, bitmap, levels, numVertices, currentLevel);

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
	printf("%s\t%s\t%"PRIu32"\t%.10lf\t%"PRIu64"\t%"PRIu64"\t%"PRIu64" \t1 \t1\n", algorithm_name, implementation_name, level, seconds[level], mispredictions[level], branches[level], instructions[level]);
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



void BFS_TopDown_Branchy_LevelInformation(uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t* level, uint32_t currRoot, uint32_t* edgesTraversed, uint32_t* queueStartPosition) {
	level[currRoot] = 0;

	queue[0] = currRoot;
	uint32_t qStart = 0, qEnd = 1;

	uint32_t currLevel=0;

	queueStartPosition[0]=0;
	uint32_t travEdge=0;
	// While queue is not empty
	while (qStart != qEnd) {
		uint64_t currElement = queue[qStart];
		uint32_t startEdge = off[currElement];
		uint32_t stopEdge = off[currElement+1];
		uint32_t nextLevel = level[currElement] + 1;
		if (level[currElement] > currLevel){
			queueStartPosition[currLevel+1] = qStart;			
			travEdge = 0;
			currLevel = level[currElement];	
//			printf("curr level %ld",currLevel); fflush(stdout);					
		}
		
		for (uint32_t j = startEdge; startEdge < stopEdge; startEdge++) {
			uint32_t k = ind[startEdge];
			edgesTraversed[currLevel]++;
			// If this is a neighbor and has not been found
			if (level[k] > level[currElement]) {
				// Checking if "k" has been found.
				if (level[k] == INT32_MAX) {
					level[k] = nextLevel; //level[currElement]+1;
					queue[qEnd++] = k;
				}
			}
		}
		qStart++;
	}
	queueStartPosition[currLevel+1] = qStart;
}

/*
void BFSSeqBranchlessSSE(int64_t* off, int64_t* ind, int64_t* queue, int64_t* level, int64_t currRoot) {
	level[currRoot] = 0;

	queue[0] = currRoot;
	int64_t qStart = 0, qEnd = 1;

	const __m128i shuffleTable[4] = {
		_mm_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
		_mm_setr_epi8(   0,    1,    2,    3,    4,    5,    6,    7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
		_mm_setr_epi8(   8,    9,   10,   11,   12,   13,   14,   15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
		_mm_setr_epi8(   0,    1,    2,    3,    4,    5,    6,    7,    8,    9,   10,   11,   12,   13,   14,   15)
	};

	// While queue is not empty
	while (qStart < qEnd) {
		uint64_t currElement;
		currElement = queue[qStart];
		qStart++;

		const int64_t startEdge = off[currElement];

		const int64_t stopEdge = off[currElement + 1];
		const int64_t nextLevel = level[currElement] + 1;
		const __m128i mmNextLevel = _mm_set1_epi64x(nextLevel);
		int64_t j;
		
		j = startEdge;
		for (; j < stopEdge - 2; j += 2) {
			const int64_t k0 = ind[j];
			const int64_t k1 = ind[j + 1];
			const __m128i mmK = _mm_set_epi64x(k1, k0);
			__m128i mmLevelK = _mm_insert_epi64(_mm_loadl_epi64((__m128i*)&level[k0]), level[k1], 1);
			const __m128i predicate = _mm_cmpgt_epi64(mmLevelK, mmNextLevel);
			const unsigned qMask = _mm_movemask_pd(_mm_castsi128_pd(predicate));
			_mm_storeu_si128((__m128i*)&queue[qEnd], _mm_shuffle_epi8(mmK, shuffleTable[qMask]));
			qEnd += __builtin_popcount(qMask);
			mmLevelK = _mm_blendv_epi8(mmLevelK, mmNextLevel, predicate);
			level[k0] = _mm_cvtsi128_si64(mmLevelK);
			level[k1] = _mm_extract_epi64(mmLevelK, 1);
		}
		for (; j < stopEdge; j ++) {
			const int64_t k = ind[j];
			int64_t levelK = level[k];
			queue[qEnd] = k;
			__asm__ __volatile__ (
				"CMPQ %[levelK], %[nextLevel];"
				"CMOVNGEQ %[nextLevel], %[levelK];"
				"ADCQ $0, %[qEnd];"
				: [qEnd] "+r" (qEnd), [levelK] "+r" (levelK)
				: [nextLevel] "r" (nextLevel)
				: "cc"
			);
			level[k] = levelK;
		}

	}
}
*/

#ifdef __SSE4_1__
	uint32_t BFS_TopDown_Branchless_SSE4_1(uint32_t* off, uint32_t* ind, const uint32_t* inputQueue, uint32_t inputVerteces, uint32_t* outputQueue, uint32_t* level, uint32_t currentLevel) {
		#define _ 0x80
		const __m128i compactionTable[16] = {
			_mm_setr_epi8(  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _ ),
			_mm_setr_epi8(  0,  1,  2,  3,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _ ),
			_mm_setr_epi8(  4,  5,  6,  7,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _ ),
			_mm_setr_epi8(  0,  1,  2,  3,  4,  5,  6,  7,  _,  _,  _,  _,  _,  _,  _,  _ ),
			_mm_setr_epi8(  8,  9, 10, 11,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _ ),
			_mm_setr_epi8(  0,  1,  2,  3,  8,  9, 10, 11,  _,  _,  _,  _,  _,  _,  _,  _ ),
			_mm_setr_epi8(  4,  5,  6,  7,  8,  9, 10, 11,  _,  _,  _,  _,  _,  _,  _,  _ ),
			_mm_setr_epi8(  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11,  _,  _,  _,  _ ),
			_mm_setr_epi8( 12, 13, 14, 15,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _ ),
			_mm_setr_epi8(  0,  1,  2,  3, 12, 13, 14, 15,  _,  _,  _,  _,  _,  _,  _,  _ ),
			_mm_setr_epi8(  4,  5,  6,  7, 12, 13, 14, 15,  _,  _,  _,  _,  _,  _,  _,  _ ),
			_mm_setr_epi8(  0,  1,  2,  3,  4,  5,  6,  7, 12, 13, 14, 15,  _,  _,  _,  _ ),
			_mm_setr_epi8(  8,  9, 10, 11, 12, 13, 14, 15,  _,  _,  _,  _,  _,  _,  _,  _ ),
			_mm_setr_epi8(  0,  1,  2,  3,  8,  9, 10, 11, 12, 13, 14, 15,  _,  _,  _,  _ ),
			_mm_setr_epi8(  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  _,  _,  _,  _ ),
			_mm_setr_epi8(  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 )
		};
		#undef _
		const __m128i currentLevelX4 = _mm_set1_epi32(currentLevel);
		uint32_t outputIndex = 0;
		while (inputVerteces--) {
			uint32_t currentVertex = *inputQueue++;
			const uint32_t startEdge = off[currentVertex];
			const uint32_t stopEdge = off[currentVertex + 1];

			uint32_t edge = startEdge;
			for (; edge + 4 <= stopEdge; edge += 4) {
				const uint32_t neighborVertex0 = ind[edge];
				const uint32_t neighborVertex1 = ind[edge + 1];
				const uint32_t neighborVertex2 = ind[edge + 2];
				const uint32_t neighborVertex3 = ind[edge + 3];
				const __m128i neightborVertices = _mm_loadu_si128((__m128i*)&ind[edge]);
				__m128i neighborLevels = _mm_unpacklo_epi64(
					_mm_insert_epi32(_mm_cvtsi32_si128(level[neighborVertex0]), level[neighborVertex1], 1),
					_mm_insert_epi32(_mm_cvtsi32_si128(level[neighborVertex2]), level[neighborVertex3], 1)
				);
				__m128i frontierPredicates = _mm_cmpgt_epi32(neighborLevels, currentLevelX4);
				const uint32_t frontierMask = _mm_movemask_ps(_mm_castsi128_ps(frontierPredicates));
				const __m128i compactedNeightborVertices = _mm_shuffle_epi8(neightborVertices, compactionTable[frontierMask]);
				_mm_storeu_si128((__m128i*)&outputQueue[outputIndex], compactedNeightborVertices);

				outputIndex += __builtin_popcount(frontierMask);
				neighborLevels = _mm_min_epi32(neighborLevels, currentLevelX4);
				level[neighborVertex0] = _mm_cvtsi128_si32(neighborLevels);
				level[neighborVertex1] = _mm_extract_epi32(neighborLevels, 1);
				level[neighborVertex2] = _mm_extract_epi32(neighborLevels, 2);
				level[neighborVertex3] = _mm_extract_epi32(neighborLevels, 3);
			}
			for (; edge != stopEdge; edge++) {
				const uint32_t neighborVertex = ind[edge];
				uint32_t neighborLevel = level[neighborVertex];
				outputQueue[outputIndex] = neighborVertex;
				__asm__ __volatile__ (
					"CMPL %[neighborLevel], %[currentLevel];"
					"CMOVNGEL %[currentLevel], %[neighborLevel];"
					"ADCL $0, %[outputIndex];"
					: [outputIndex] "+r" (outputIndex), [neighborLevel] "+r" (neighborLevel)
					: [currentLevel] "r" (currentLevel)
					: "cc"
				);
				level[neighborVertex] = neighborLevel;
			}
		}
		return outputIndex;
	}

	//~ void BFS_TopDown_Branchless_SSE4_1(uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t* level, uint32_t currRoot) {
		//~ level[currRoot] = 0;

		//~ queue[0] = currRoot;
		//~ uint32_t qStart = 0, qEnd = 1;

		//~ #define _ 0x80
		//~ const __m128i shuffleTable[16] = {
			//~ _mm_setr_epi8(  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8(  0,  1,  2,  3,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8(  4,  5,  6,  7,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8(  0,  1,  2,  3,  4,  5,  6,  7,  _,  _,  _,  _,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8(  8,  9, 10, 11,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8(  0,  1,  2,  3,  8,  9, 10, 11,  _,  _,  _,  _,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8(  4,  5,  6,  7,  8,  9, 10, 11,  _,  _,  _,  _,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8(  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8( 12, 13, 14, 15,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8(  0,  1,  2,  3, 12, 13, 14, 15,  _,  _,  _,  _,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8(  4,  5,  6,  7, 12, 13, 14, 15,  _,  _,  _,  _,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8(  0,  1,  2,  3,  4,  5,  6,  7, 12, 13, 14, 15,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8(  8,  9, 10, 11, 12, 13, 14, 15,  _,  _,  _,  _,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8(  0,  1,  2,  3,  8,  9, 10, 11, 12, 13, 14, 15,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8(  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8(  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 )
		//~ };
		//~ #undef _

		//~ // While queue is not empty
		//~ while (qStart < qEnd) {
			//~ uint32_t currElement = queue[qStart];
			//~ qStart++;

			//~ const uint32_t startEdge = off[currElement];

			//~ const uint32_t stopEdge = off[currElement + 1];
			//~ const uint32_t nextLevel = level[currElement] + 1;
			//~ const __m128i mmNextLevel = _mm_set1_epi32(nextLevel);
			
			//~ uint32_t j = startEdge;
			//~ for (; j + 4 < stopEdge; j += 4) {
				//~ const uint32_t k0 = ind[j];
				//~ const uint32_t k1 = ind[j + 1];
				//~ const uint32_t k2 = ind[j + 2];
				//~ const uint32_t k3 = ind[j + 3];
				//~ const __m128i mmK = _mm_loadu_si128((__m128i*)&ind[j]);
				//~ __m128i mmLevelK = _mm_insert_epi32(_mm_insert_epi32(_mm_insert_epi32(_mm_cvtsi32_si128(level[k0]), level[k1], 1), level[k2], 2), level[k3], 3);
				//~ __m128i predicate = _mm_cmpgt_epi32(mmLevelK, mmNextLevel);
				//~ const unsigned qMask = _mm_movemask_ps(_mm_castsi128_ps(predicate));
				//~ //const unsigned qMask = _mm_movemask_epi8(predicate);

				//~ //predicate = _mm_and_si128(predicate, _mm_set1_epi8(4));
				//~ //__m128i mask = predicate;
				//~ /*
				//~ mask = _mm_add_epi8(mask, _mm_srli_si128(predicate, 4));
				//~ mask = _mm_add_epi8(mask, _mm_srli_si128(predicate, 8));
				//~ mask = _mm_add_epi8(mask, _mm_srli_si128(predicate, 12));
				//~ */
				//~ //~ mask = _mm_add_epi8(mask, _mm_srli_si128(mask, 4));
				//~ //~ mask = _mm_add_epi8(mask, _mm_srli_si128(mask, 8));
				//~ //~ mask = _mm_add_epi8(mask, _mm_setr_epi8(0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3));
				//~ //const __m128i compressed = _mm_shuffle_epi8(mmK, mask);
				//~ const __m128i compressed = _mm_shuffle_epi8(mmK, shuffleTable[qMask]);
				//~ _mm_storeu_si128((__m128i*)&queue[qEnd], compressed);

				//~ qEnd += __builtin_popcount(qMask);
				//~ //~ qEnd += __builtin_popcount(qMask) >> 2;
				//~ mmLevelK = _mm_min_epi32(mmLevelK, mmNextLevel);
				//~ level[k0] = _mm_cvtsi128_si32(mmLevelK);
				//~ level[k1] = _mm_extract_epi32(mmLevelK, 1);
				//~ level[k2] = _mm_extract_epi32(mmLevelK, 2);
				//~ level[k3] = _mm_extract_epi32(mmLevelK, 3);
			//~ }
			//~ for (; j < stopEdge; j ++) {
				//~ const uint32_t k = ind[j];
				//~ uint32_t levelK = level[k];
				//~ queue[qEnd] = k;
				//~ __asm__ __volatile__ (
					//~ "CMPL %[levelK], %[nextLevel];"
					//~ "CMOVNGEL %[nextLevel], %[levelK];"
					//~ "ADCL $0, %[qEnd];"
					//~ : [qEnd] "+r" (qEnd), [levelK] "+r" (levelK)
					//~ : [nextLevel] "r" (nextLevel)
					//~ : "cc"
				//~ );
				//~ level[k] = levelK;
			//~ }

		//~ }
	//~ }
#endif

#ifdef __AVX2__
	void BFS_TopDown_Branchless_AVX2(uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t* level, uint32_t currRoot) {
		level[currRoot] = 0;

		queue[0] = currRoot;
		uint32_t qStart = 0, qEnd = 1;

		const __m256i shuffleTable[256] = {
			_mm256_setr_epi32( 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    3, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    4, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    4, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    4, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    4, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    4, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    4, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    4, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    4, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    3,    4, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    4, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    4, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    4, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3,    4, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    4, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    4, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    4, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    5, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    5, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    5, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    5, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    5, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    5, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    5, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    5, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    3,    5, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    5, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    5, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    5, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3,    5, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    5, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    5, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    5, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    4,    5, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    4,    5, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    4,    5, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    4,    5, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    4,    5, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    4,    5, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    4,    5, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    4,    5, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    3,    4,    5, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    4,    5, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    4,    5, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    4,    5, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3,    4,    5, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    4,    5, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    4,    5, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    4,    5, 0x80, 0x80),
			_mm256_setr_epi32(    6, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    6, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    6, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    6, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    3,    6, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    4,    6, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    4,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    4,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    4,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    4,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    4,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    4,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    4,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    3,    4,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    4,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    4,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    4,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3,    4,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    4,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    4,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    4,    6, 0x80, 0x80),
			_mm256_setr_epi32(    5,    6, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    5,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    5,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    5,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    5,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    5,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    5,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    5,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    3,    5,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    5,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    5,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    5,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3,    5,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    5,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    5,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    5,    6, 0x80, 0x80),
			_mm256_setr_epi32(    4,    5,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    4,    5,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    4,    5,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    4,    5,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    4,    5,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    4,    5,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    4,    5,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    4,    5,    6, 0x80, 0x80),
			_mm256_setr_epi32(    3,    4,    5,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    4,    5,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    4,    5,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    4,    5,    6, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3,    4,    5,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    4,    5,    6, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    4,    5,    6, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    4,    5,    6, 0x80),
			_mm256_setr_epi32(    7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    3,    7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    4,    7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    4,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    4,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    4,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    4,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    4,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    4,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    4,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    3,    4,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    4,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    4,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    4,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3,    4,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    4,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    4,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    4,    7, 0x80, 0x80),
			_mm256_setr_epi32(    5,    7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    5,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    5,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    5,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    5,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    5,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    5,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    5,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    3,    5,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    5,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    5,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    5,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3,    5,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    5,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    5,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    5,    7, 0x80, 0x80),
			_mm256_setr_epi32(    4,    5,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    4,    5,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    4,    5,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    4,    5,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    4,    5,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    4,    5,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    4,    5,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    4,    5,    7, 0x80, 0x80),
			_mm256_setr_epi32(    3,    4,    5,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    4,    5,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    4,    5,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    4,    5,    7, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3,    4,    5,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    4,    5,    7, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    4,    5,    7, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    4,    5,    7, 0x80),
			_mm256_setr_epi32(    6,    7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    6,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    6,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    6,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    3,    6,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    4,    6,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    4,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    4,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    4,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    4,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    4,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    4,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    4,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    3,    4,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    4,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    4,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    4,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3,    4,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    4,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    4,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    4,    6,    7, 0x80),
			_mm256_setr_epi32(    5,    6,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    5,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    5,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    5,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    5,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    5,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    5,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    5,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    3,    5,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    5,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    5,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    5,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3,    5,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    5,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    5,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    5,    6,    7, 0x80),
			_mm256_setr_epi32(    4,    5,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    4,    5,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    4,    5,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    4,    5,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    2,    4,    5,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    4,    5,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    4,    5,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    4,    5,    6,    7, 0x80),
			_mm256_setr_epi32(    3,    4,    5,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    4,    5,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    4,    5,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    4,    5,    6,    7, 0x80),
			_mm256_setr_epi32(    2,    3,    4,    5,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    4,    5,    6,    7, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    4,    5,    6,    7, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    4,    5,    6,    7)
		};

		// While queue is not empty
		while (qStart < qEnd) {
			uint32_t currElement = queue[qStart];
			qStart++;

			const uint32_t startEdge = off[currElement];

			const uint32_t stopEdge = off[currElement + 1];
			const uint32_t nextLevel = level[currElement] + 1;
			const __m256i mmNextLevel = _mm256_set1_epi32(nextLevel);
			uint32_t j;
			
			j = startEdge;
			for (; j + 8 < stopEdge; j += 8) {
				const uint32_t k0 = ind[j];
				const uint32_t k1 = ind[j + 1];
				const uint32_t k2 = ind[j + 2];
				const uint32_t k3 = ind[j + 3];
				const uint32_t k4 = ind[j + 4];
				const uint32_t k5 = ind[j + 5];
				const uint32_t k6 = ind[j + 6];
				const uint32_t k7 = ind[j + 7];
				const __m256i mmK = _mm256_loadu_si256((__m256i*)&ind[j]);
				__m256i mmLevelK = _mm256_inserti128_si256(
					_mm256_castsi128_si256(_mm_insert_epi32(_mm_insert_epi32(_mm_insert_epi32(_mm_cvtsi32_si128(level[k0]), level[k1], 1), level[k2], 2), level[k3], 3)),
					_mm_insert_epi32(_mm_insert_epi32(_mm_insert_epi32(_mm_cvtsi32_si128(level[k4]), level[k5], 1), level[k6], 2), level[k7], 3), 1);
				const __m256i predicate = _mm256_cmpgt_epi32(mmLevelK, mmNextLevel);
				const unsigned qMask = _mm256_movemask_ps(_mm256_castsi256_ps(predicate));
				_mm256_storeu_si256((__m256i*)&queue[qEnd],  _mm256_permutevar8x32_epi32(mmK, shuffleTable[qMask]));
				qEnd += __builtin_popcount(qMask);
				mmLevelK = _mm256_min_epi32(mmLevelK, mmNextLevel);
				const __m128i mmLevelKLo = _mm256_castsi256_si128(mmLevelK);
				const __m128i mmLevelKHi = _mm256_extracti128_si256(mmLevelK, 1);
				level[k0] = _mm_cvtsi128_si32(mmLevelKLo);
				level[k1] = _mm_extract_epi32(mmLevelKLo, 1);
				level[k2] = _mm_extract_epi32(mmLevelKLo, 2);
				level[k3] = _mm_extract_epi32(mmLevelKLo, 3);
				level[k4] = _mm_cvtsi128_si32(mmLevelKHi);
				level[k5] = _mm_extract_epi32(mmLevelKHi, 1);
				level[k6] = _mm_extract_epi32(mmLevelKHi, 2);
				level[k7] = _mm_extract_epi32(mmLevelKHi, 3);
			}
			for (; j < stopEdge; j ++) {
				const uint32_t k = ind[j];
				uint32_t levelK = level[k];
				queue[qEnd] = k;
				__asm__ __volatile__ (
					"CMPL %[levelK], %[nextLevel];"
					"CMOVNGEL %[nextLevel], %[levelK];"
					"ADCL $0, %[qEnd];"
					: [qEnd] "+r" (qEnd), [levelK] "+r" (levelK)
					: [nextLevel] "r" (nextLevel)
					: "cc"
				);
				level[k] = levelK;
			}

		}
		//~ printf("\nQE %d\n",qEnd);
	}
#endif

#ifdef __MIC__
	void BFS_TopDown_Branchless_MIC(uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t* level, uint32_t currRoot) {
		level[currRoot] = 0;

		queue[0] = currRoot;
		uint32_t qStart = 0, qEnd = 1;

		// While queue is not empty
		while (qStart < qEnd) {
			uint32_t currElement = queue[qStart];
			qStart++;

			const uint32_t startEdge = off[currElement];

			const uint32_t stopEdge = off[currElement + 1];
			const uint32_t nextLevel = level[currElement] + 1;
			const __m512i mmNextLevel = _mm512_set1_epi32(nextLevel);
			
			uint32_t j = startEdge;
			for (; j + 16 < stopEdge; j += 16) {
				__m512i k = _mm512_undefined_epi32();
				k = _mm512_loadunpacklo_epi32(k, &ind[j]);
				k = _mm512_loadunpackhi_epi32(k, &ind[j + 16]);
				__m512i levelK = _mm512_i32gather_epi32(k, level, sizeof(uint32_t));
				const __mmask16 predicate = _mm512_cmp_epi32_mask(levelK, mmNextLevel, _MM_CMPINT_GT);
				_mm512_mask_packstorelo_epi32(&queue[qEnd], predicate, k);
				_mm512_mask_packstorehi_epi32(&queue[qEnd + 16], predicate, k);
				qEnd += _mm_countbits_32(_mm512_mask2int(predicate));
				levelK = _mm512_min_epi32(levelK, mmNextLevel);
				_mm512_i32scatter_epi32(level, k, levelK, sizeof(uint32_t));
			}
			if (j != stopEdge) {
				const __mmask16 elements_mask = _mm512_int2mask((1 << (stopEdge - j)) - 1);
				__m512i k = _mm512_undefined_epi32();
				k = _mm512_mask_loadunpacklo_epi32(k, elements_mask, &ind[j]);
				k = _mm512_mask_loadunpackhi_epi32(k, elements_mask, &ind[j + 16]);
				__m512i levelK = _mm512_mask_i32gather_epi32(_mm512_undefined_epi32(), elements_mask, k, level, sizeof(uint32_t));
				const __mmask16 predicate = _mm512_mask_cmp_epi32_mask(elements_mask, levelK, mmNextLevel, _MM_CMPINT_GT);
				_mm512_mask_packstorelo_epi32(&queue[qEnd], predicate, k);
				_mm512_mask_packstorehi_epi32(&queue[qEnd + 16], predicate, k);
				qEnd += _mm_countbits_32(_mm512_mask2int(predicate));
				levelK = _mm512_mask_min_epi32(_mm512_undefined_epi32(), elements_mask, levelK, mmNextLevel);
				_mm512_mask_i32scatter_epi32(level, elements_mask, k, levelK, sizeof(uint32_t));
			}
		}
		//~ printf("\nQE %d\n",qEnd);
	}
#endif
