#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <inttypes.h>
#include <assert.h>
#include <time.h>
#include <math.h>

#include "main.h"




#if defined(BENCHMARK_BC)

void Benchmark_BC(const char* algorithm_name, const char* implementation_name, const struct PerformanceCounter performanceCounters[], size_t performanceCounterCount, BFS_TopDown_Function bfs_function, uint32_t numVertices, uint32_t* off, uint32_t* ind, uint32_t* edgesTraversed) {
  struct perf_event_attr perf_counter;

  uint32_t* queue = (uint32_t*)memalign(64, numVertices * sizeof(uint32_t));
  uint32_t* level = (uint32_t*)memalign(64, numVertices * sizeof(uint32_t));
  uint32_t* sigma = (uint32_t*)memalign(64, numVertices * sizeof(uint32_t));
  uint32_t* delta = (uint32_t*)memalign(64, numVertices * sizeof(uint32_t));
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
  free(sigma);
  free(delta);
  free(perf_events);
  free(vertices);

}


#endif


// uint64_t bcTreeBranchBased(bcForest* forest, struct stinger* sStinger,
// 		uint64_t currRoot, bc_t* totalBC,extraArraysPerThread* eAPT){
// 	bcTree* tree = forest->forest[currRoot];

// 	for(uint64_t j = 0; j < tree->NV; j++){
// 		tree->vArr[j].level = INFINITY_MY;
// 		tree->vArr[j].sigma = INFINITY_MY;
// 		tree->vArr[j].delta = 0;
// 	}
// 	tree->vArr[currRoot].level = 0;
// 	tree->vArr[currRoot].sigma = 1;

// 	uint64_t* Stack= eAPT->Stack;
// 	uint64_t* Queue = eAPT->QueueDown;

// 	Queue[0] = currRoot;
// 	int64_t qStart=0,qEnd=1;
// 	int64_t sStart=0;
// 	int64_t k;
// 	// While queue is not empty
// 	while(qStart!=qEnd)	{
// 		uint64_t currElement = Queue[qStart];
// 		Stack[sStart] = currElement;
// 		sStart++;
// 		qStart++;

// 		STINGER_FORALL_EDGES_OF_VTX_BEGIN(sStinger,currElement){
// 			k = STINGER_EDGE_DEST;
// 			// If this is a neighbor and has not been found
// 			if(tree->vArr[k].level > tree->vArr[currElement].level){
// 				// Checking if "k" has been found.
// 				if(tree->vArr[k].level==INFINITY_MY){
// 					tree->vArr[k].level = tree->vArr[currElement].level+1;
// 					Queue[qEnd++] = k;
// 					tree->vArr[k].delta=0;
// 				}

// 				if(tree->vArr[k].sigma == INFINITY_MY){
// 					// k has not been found and therefore its paths to the roots are through its parent.
// 					tree->vArr[k].sigma = tree->vArr[currElement].sigma;
// 				}
// 				else{
// 					// k has been found and has multiple paths to the root as it has multiple parents.
// 					tree->vArr[k].sigma += tree->vArr[currElement].sigma;
// 				}
// 			}
// 		}
// 		STINGER_FORALL_EDGES_OF_VTX_END();
// 	}

// 	// Using Brandes algorithm to compute BC for a specific tree.
// 	// Essentially, use the stack which the elements are placed in depth-reverse order, to "climb" back
// 	// up the tree, all the way to the root.
// 	int64_t sEnd = sStart-1;
// 	while(sEnd>=0){
// 		uint64_t currElement = Stack[sEnd];

// 		STINGER_FORALL_EDGES_OF_VTX_BEGIN(sStinger,currElement){
// 			k = STINGER_EDGE_DEST;
// 			// If this is a neighbor and has not been found
// 			if((tree->vArr[k].level == (tree->vArr[currElement].level-1))){
// 				tree->vArr[k].delta +=
// 					((bc_t)tree->vArr[k].sigma/(bc_t)tree->vArr[currElement].sigma)*
// 					(bc_t)(tree->vArr[currElement].delta+1);
// 			}
// 		}
// 		STINGER_FORALL_EDGES_OF_VTX_END();
// 		if(currElement!=currRoot){
// 			eAPT->sV[currElement].totalBC+=tree->vArr[currElement].delta;
// 		}
// 		sEnd--;
// 	}
// 	return -1;
// }
