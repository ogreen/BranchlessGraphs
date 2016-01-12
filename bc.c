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
	#include <xmmintrin.h>
	#include <immintrin.h>
#endif


#include "main.h"

uint32_t bcTreeBranchBased(uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t inputStart, uint32_t inputNum, 
		uint32_t outputStart, uint32_t* level,uint32_t* sigma, float*delta)
{
	uint32_t* outputQueue=queue+outputStart;
	int32_t qPrevStart=inputStart,qPrevEnd=inputStart+inputNum;
	int32_t qOut=0;
	// int32_t stackPos=0;
	int32_t k;
	// printf("%d %d\n", qPrevStart, qPrevEnd);
	// While queue is not empty
	while(qPrevStart!=qPrevEnd)	{
		uint32_t currElement = queue[qPrevStart++];
		// stack[stackPos] = currElement;
		// stackPos++;
		uint32_t currentLevel= level[currElement];
		uint32_t nextLevel = level[currElement]+1;

		uint32_t startEdge = off[currElement];
		uint32_t stopEdge = off[currElement+1];
		for (uint32_t j = startEdge; startEdge < stopEdge; startEdge++) {
			uint32_t k = ind[startEdge];
			// Checking if "k" has been found.
			if(level[k]==INT32_MAX){
				level[k] = nextLevel;
				outputQueue[qOut++] = k;
				delta[k]=0;
			}
			if(level[k]==(level[currElement]+1)){
				sigma[k] += sigma[currElement];
			}
		}

	}
	return 	qOut;
}


uint32_t bcTreeBranchAvoiding(uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t inputStart, uint32_t inputNum, 
		uint32_t outputStart, uint32_t* level,uint32_t* sigma, float*delta)
{
	uint32_t* outputQueue=queue+outputStart;
	int32_t qPrevStart=inputStart,qPrevEnd=inputStart+inputNum;
	int32_t qOut=0;
	// int32_t stackPos=0;
	int32_t k;
	// printf("%d %d\n", qPrevStart, qPrevEnd);
	// While queue is not empty
	while(qPrevStart!=qPrevEnd)	{
		uint32_t currElement = queue[qPrevStart++];
		// stack[stackPos] = currElement;
		// stackPos++;
		uint32_t currentLevel= level[currElement];
		uint32_t nextLevel = level[currElement]+1;

		uint32_t startEdge = off[currElement];
		uint32_t stopEdge = off[currElement+1];

		for (uint32_t j = startEdge; startEdge < stopEdge; startEdge++) {
			uint32_t k = ind[startEdge];
			// Checking if "k" has been found.
			if(level[k]==INT32_MAX){
				level[k] = nextLevel;
				outputQueue[qOut++] = k;
				delta[k]=0;
			}

//			sigma[k] += ((level[k]-currentLevel)>0)*sigma[currElement];

#if defined(X86)
			int sigmacurr=sigma[currElement];
			int levelk=level[k];
			int tempVal=0;
			__asm__ __volatile__ (
				"CMP %[levelk], %[nextLevel];"
				"CMOVG %[tempVal], %[sigmacurr];"
				: [tempVal] "+r" (tempVal)
				: [levelk] "r" (levelk), [nextLevel] "r" (nextLevel),[sigmacurr] "r" (sigmacurr)
			);
			sigma[k]+=tempVal;
#endif

#if defined(ARMASM)
			int sigmak=sigma[k];
			int sigmacurr=sigma[currElement];
			int levelk=level[k];
			__asm__ __volatile__ (
				"CMP %[levelk], %[nextLevel];"
				"ADDHI %[sigmak], %[sigmacurr];"
				: [sigmak] "+r" (sigmak)
				: [levelk] "r" (levelk), [nextLevel] "r" (nextLevel),[sigmacurr] "r" (sigmacurr)
			);
#endif

		}

	}
	return 	qOut;
}


void bcDependencyBranchBased(uint32_t currRoot,uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t reverseStart, uint32_t numElements, 
		uint32_t* level,uint32_t* sigma, float*delta, float* totalBC)
{

	uint32_t* reverseQueue=queue;
	int32_t startPos=reverseStart,leftOver=numElements;

	// Using Brandes algorithm to compute BC for a specific tree.
	// Essentially, use the stack which the elements are placed in depth-reverse order, to "climb" back
	// up the tree, all the way to the root.
	// int32_t sEnd = stackPos-1;
	while(leftOver>=0){
		uint32_t currElement = reverseQueue[leftOver];

		uint32_t startEdge = off[currElement];
		uint32_t stopEdge = off[currElement+1];
		uint32_t prevLevel = level[currElement]-1;
		for (uint32_t j = startEdge; startEdge < stopEdge; startEdge++) {
			uint32_t k = ind[startEdge];
			// If this is a neighbor and has not been found
			if(level[k] == prevLevel){
				delta[k] += ((float)sigma[k]/(float)sigma[currElement])*(float)(delta[currElement]+1);
			}
		}
		if(currElement!=currRoot){
			totalBC[currElement]+=delta[currElement];
		}
		leftOver--;
	}
}

void bcDependencyBranchAvoiding(uint32_t currRoot,uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t reverseStart, uint32_t numElements, 
		uint32_t* level,uint32_t* sigma, float*delta, float* totalBC)
{

	uint32_t* reverseQueue=queue;
	int32_t startPos=reverseStart,leftOver=numElements;

	// Using Brandes algorithm to compute BC for a specific tree.
	// Essentially, use the stack which the elements are placed in depth-reverse order, to "climb" back
	// up the tree, all the way to the root.
	// int32_t sEnd = stackPos-1;
	while(leftOver>=0){
		uint32_t currElement = reverseQueue[leftOver];

		uint32_t startEdge = off[currElement];
		uint32_t stopEdge = off[currElement+1];
		uint32_t currLevel = level[currElement]; 
		float sigmadivdelta = (float)(sigma[currElement])/(float)(delta[currElement]+1);
		for (uint32_t j = startEdge; startEdge < stopEdge; startEdge++) {
			uint32_t k = ind[startEdge];
			// If this is a neighbor and has not been found
			// if(level[k] == prevLevel){
			// 	delta[k] += ((float)sigma[k]/(float)sigma[currElement])*(float)(delta[currElement]+1);
			// }
//			delta[k] += ((currLevel-level[k])>0)*((float)sigma[k]/(float)sigmadivdelta);			

#if defined(X86)

			float sigmak=sigma[k];
			__m128 mmSDD = _mm_load_ps1 (&sigmadivdelta);
			__m128 deltak = _mm_load_ps1 (delta+k);
			__m128 mmsigmak = _mm_load_ps1 (&sigmak);

			__m128 temp=_mm_add_ps(deltak,_mm_mul_ps(mmSDD,mmsigmak));
			float temp_float[4]={0,0,0,0}; _mm_store_ps1 (temp_float,temp);
			delta[k]+=temp_float[0];
            // Conditional store still needs to be applied.
#endif


#if defined(ARMASM)
/*			int levelk=level[k];
			float sigmak=sigma[k];
			float deltak=delta[k];
			//float temp1=0;// temp2=1, temp3=0;
			
			__asm__ __volatile__ ( ""
				"CMP %[currLevel], %[levelk];"
				"VMLAHI.F32 %[deltak], %[sigmak], %[sigmadivdelta];"

//			    "VMULHI.F32 %[temp1], %[sigmak], %[sigmadivdelta];"
//			    "VADDHI.F32 %[deltak], %[deltak], %[temp1];"
//				: [temp1] "+w" (temp1) , [deltak] "+w" (deltak)
				: [deltak] "+w" (deltak)
				: [levelk] "r" (levelk), [currLevel] "r" (currLevel), [sigmak] "w" (sigmak), [sigmadivdelta] "w" (sigmadivdelta) 
			);
*/

 			int levelk=level[k];
//			float sigmak=sigma[k];
//			float deltak=delta[k];
//			int tempval = delta+k;			
			__asm__ __volatile__ ( ""
				"CMP %[currLevel], %[levelk];"
//				"VMLAHI.F32 %[deltak], %[sigmak], %[sigmadivdelta];"
 				:// [deltak] "+w" (deltak)
				: [levelk] "r" (levelk), [currLevel] "r" (currLevel)//, [sigmak] "w" (sigmak), [sigmadivdelta] "w" (sigmadivdelta) 
			);
 

#endif
 

		}
		if(currElement!=currRoot){
			totalBC[currElement]+=delta[currElement];
		}
		leftOver--;
	}

}




#if defined(BENCHMARK_BC)

void Benchmark_BC(const char* algorithm_name, const char* implementation_name, const struct PerformanceCounter performanceCounters[], size_t performanceCounterCount, BC_TRAV_Function bc_trav_function,  BC_DEP_Function bc_dep_function, uint32_t numVertices, uint32_t* off, uint32_t* ind, uint32_t* edgesTraversed) {
	struct perf_event_attr perf_counter;

	const uint32_t rootVertex = 1;

	uint32_t* queue = (uint32_t*)memalign(64, numVertices * sizeof(uint32_t));
	uint32_t* stack = (uint32_t*)memalign(64, numVertices * sizeof(uint32_t));
	uint32_t* level = (uint32_t*)memalign(64, numVertices * sizeof(uint32_t));
	uint32_t* sigma = (uint32_t*)memalign(64, numVertices * sizeof(uint32_t));
	float*    delta = (float*)memalign(64, numVertices * sizeof(float));
	float*    totalBC = (float*)memalign(64, numVertices * sizeof(float));
	uint32_t* perf_events = (uint32_t*)malloc(numVertices * sizeof(uint32_t));
	uint32_t* vertices = (uint32_t*)malloc(numVertices * sizeof(uint32_t));
	uint32_t* verticesPrefixSum = (uint32_t*)malloc(numVertices * sizeof(uint32_t));

	int32_t levelCount = 0;
	uint32_t totalVertices = 0;
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
			sigma[i] = 0;
			delta[i] = 0;
			queue[i] = 0;
			totalBC[i]=0;
		}

		uint32_t currentLevel = 0;
		uint32_t queuePosition=0;

		level[rootVertex] = currentLevel;
		sigma[rootVertex] = 1;
		queue[0] = rootVertex;

		currentLevel++;

		uint32_t outputVertices = 1;
		do {
			const uint32_t inputVertices = outputVertices;
			if (levelCount == 0) {
				vertices[currentLevel-1] = inputVertices;
				totalVertices+=inputVertices;
		  	}

			struct timespec startTime;
			if (performanceCounters[performanceCounterIndex].type == PERF_TYPE_TIME) {
				assert(clock_gettime(CLOCK_MONOTONIC, &startTime) == 0);
			} else {
				assert(ioctl(perf_counter_fd, PERF_EVENT_IOC_RESET, 0) == 0);
				assert(ioctl(perf_counter_fd, PERF_EVENT_IOC_ENABLE, 0) == 0);
			}
			outputVertices = bc_trav_function(
				off, 
				ind,
				queue,
				queuePosition,
				outputVertices,
				queuePosition+outputVertices,
				level,
				sigma,
				delta
			);

			queuePosition += inputVertices;

			if (performanceCounters[performanceCounterIndex].type == PERF_TYPE_TIME) {
				struct timespec endTime;
				assert(clock_gettime(CLOCK_MONOTONIC, &endTime) == 0);
				perf_events[levelCount * performanceCounterIndex + (currentLevel-1)] =
				  (1000000000ll * endTime.tv_sec + endTime.tv_nsec) - 
				  (1000000000ll * startTime.tv_sec + startTime.tv_nsec);
			} 
			else {
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
	  		printf("\t%"PRIu32, perf_events[levelCount * performanceCounterIndex + level]);
		}
		printf("\t%"PRIu32"\t%"PRIu32"\n", vertices[level], edgesTraversed[level]);
	}
	

	int sum=vertices[0];
	verticesPrefixSum[0]=vertices[0];	
	for(int i=1; i <levelCount; i++){
		sum+=vertices[i];
		verticesPrefixSum[i]=verticesPrefixSum[i-1]+vertices[i];
	}
	// for(int i=1; i <levelCount+1; i++){
	// 	printf("%d, ",verticesPrefixSum[i]);
	// }
	// printf("\n");
	// printf("total %d %d\n", totalVertices, sum);

///----------------------------
	free(perf_events);	
	perf_events = (uint32_t*)malloc(numVertices * sizeof(uint32_t));

	int totalLevelCount=levelCount;
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

		int currentLevel=totalLevelCount-1;
		do {
			struct timespec startTime;
			if (performanceCounters[performanceCounterIndex].type == PERF_TYPE_TIME) {
				assert(clock_gettime(CLOCK_MONOTONIC, &startTime) == 0);
			} else {
				assert(ioctl(perf_counter_fd, PERF_EVENT_IOC_RESET, 0) == 0);
				assert(ioctl(perf_counter_fd, PERF_EVENT_IOC_ENABLE, 0) == 0);
			}
			bc_dep_function(
				rootVertex,
				off, 
				ind,
				queue,
				verticesPrefixSum[currentLevel],
				vertices[currentLevel],
				level,
				sigma,
				delta,
				totalBC
			);

			// queuePosition += inputVertices;

			if (performanceCounters[performanceCounterIndex].type == PERF_TYPE_TIME) {
				struct timespec endTime;
				assert(clock_gettime(CLOCK_MONOTONIC, &endTime) == 0);
				perf_events[levelCount * performanceCounterIndex + (currentLevel)] =
				  (1000000000ll * endTime.tv_sec + endTime.tv_nsec) - 
				  (1000000000ll * startTime.tv_sec + startTime.tv_nsec);
			} 
			else {
				assert(ioctl(perf_counter_fd, PERF_EVENT_IOC_DISABLE, 0) == 0);
				// assert(read(perf_counter_fd, &perf_events[levelCount * performanceCounterIndex + (currentLevel-1)], sizeof(uint64_t)) == sizeof(uint64_t));
				int64_t temp;
				assert(read(perf_counter_fd, &temp, sizeof(uint64_t)) == sizeof(uint64_t));
				perf_events[levelCount * performanceCounterIndex + (currentLevel)]=temp;
				// printf("%d %d %ld \n,", levelCount, performanceCounterIndex,tempbs);

			}
			currentLevel--;
		} while (currentLevel>=0);
		if (levelCount == 0) {
			levelCount=totalLevelCount;
			perf_events = realloc(perf_events, numVertices * sizeof(uint64_t) * (levelCount+1));
		}
		close(perf_counter_fd);
		// printf("\n");
	}

	for (int32_t level = levelCount-1; level >=0 ; level--) {
		printf("%s\t%s\t%"PRIu32, algorithm_name, implementation_name, level);
		for (size_t performanceCounterIndex = 0; performanceCounterIndex < performanceCounterCount; performanceCounterIndex++) {
		 	if (!performanceCounters[performanceCounterIndex].supported)
				continue;
	  		printf("\t%"PRIu32, perf_events[levelCount * performanceCounterIndex + level]);
		}
		printf("\t%"PRIu32"\t%"PRIu32"\n", vertices[level], edgesTraversed[level]);
	}
	
	free(perf_events);



	free(queue);
	free(stack);
	free(level);
	free(sigma);
	free(delta);
	free(totalBC);
	free(vertices);
	free(verticesPrefixSum);
}


#endif

/*
(position of input queue == const uint32_t* inputQueue, )
(number of input vertices == uint32_t inputVertices, )
(position of outout queue == uint32_t* outputQueue, )
### (level array uint32_t* levels, )
(current levell == uint32_t currentLevel);)
*/




