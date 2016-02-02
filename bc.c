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


#if defined(ARMASM)
  #include <arm_neon.h>
#endif

#include "main.h"

uint32_t bcTreeBranchBasedSOA(uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t inputStart, uint32_t inputNum, 
		uint32_t outputStart, void* bcSOAStruct)//uint32_t* level,uint32_t* sigma, float*delta);
{
	uint32_t* level = ((bcSOA*)bcSOAStruct)->level;
	uint32_t* sigma = ((bcSOA*)bcSOAStruct)->sigma;
	float* delta = ((bcSOA*)bcSOAStruct)->delta;

	uint32_t* outputQueue=queue+outputStart;
	int32_t qPrevStart=inputStart,qPrevEnd=inputStart+inputNum;
	int32_t qOut=0;
	int32_t k;
	// While queue is not empty
	while(qPrevStart!=qPrevEnd)	{
		uint32_t currElement = queue[qPrevStart++];
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
			if(level[k]==(nextLevel))
				sigma[k] += sigma[currElement];
		}
	}
	return 	qOut;
}


uint32_t bcTreeBranchAvoidingSOA(uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t inputStart, uint32_t inputNum, 
		uint32_t outputStart, void* bcSOAStruct)//uint32_t* level,uint32_t* sigma, float*delta);
{
	uint32_t* level = ((bcSOA*)bcSOAStruct)->level;
	uint32_t* sigma = ((bcSOA*)bcSOAStruct)->sigma;
	float* delta = ((bcSOA*)bcSOAStruct)->delta;

	uint32_t* outputQueue=queue+outputStart;
	int32_t qPrevStart=inputStart,qPrevEnd=inputStart+inputNum;
	int32_t qOut=0;
	int32_t k;
	// While queue is not empty
	while(qPrevStart!=qPrevEnd)	{
		uint32_t currElement = queue[qPrevStart++];
		int32_t nextLevel = level[currElement]+1;

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

#if defined(X86)
            int32_t levelk=level[k];
			int32_t sigmaCurr = sigma[currElement];
			int32_t tempVal=0;
   		__asm__ __volatile__ (
				"CMP %[levelk], %[nextLevel];      \n\t"
				"CMOVE %[sigmaCurr], %[tempVal];   \n\t"
		    	: [tempVal] "+r" (tempVal)
				: [levelk] "r" (levelk), 
				  [nextLevel] "r" (nextLevel),
				  [sigmaCurr] "r" (sigmaCurr)
			);
		   sigma[k]+=tempVal;		
#endif

#if defined(ARMASM)
			int sigmak;//=sigma[k];
			int sigmacurr;//=sigma[currElement];
			int levelk=level[k];
			__asm__ __volatile__ (
				"CMP %[nextLevel], %[levelk];                \n\t"
				"LDREQ %[sigmak], [%[sigma], %[k], LSL #2];  \n\t"
				"LDREQ %[sigmacurr], [%[sigma], %[currElement], LSL #2];  \n\t"
				"ADDEQ %[sigmak],%[sigmak], %[sigmacurr];              \n\t"
				"STREQ %[sigmak], [%[sigma], %[k], LSL #2];  \n\t"
				: 
				[sigmak] "+r" (sigmak),
				[sigmacurr] "+r" (sigmacurr)
				: 
				[levelk] "r" (levelk), 
				[nextLevel] "r" (nextLevel),				
				[sigma] "r" (sigma),
				[currElement] "r" (currElement),
				[k] "r" (k)
			);
#endif
		}
	}
	return 	qOut;
}


void bcDependencyBranchBasedSOA(uint32_t currRoot,uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t reverseStart, uint32_t numElements, 
void* bcSOAStruct, float* totalBC) //	uint32_t* level,uint32_t* sigma, float*delta, float* totalBC)
{
	uint32_t* level = ((bcSOA*)bcSOAStruct)->level;
	uint32_t* sigma = ((bcSOA*)bcSOAStruct)->sigma;
	float* delta = ((bcSOA*)bcSOAStruct)->delta;


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
		float deltadivsigma = (float)(delta[currElement]+1)/(float)(sigma[currElement]);
		
		for (uint32_t j = startEdge; startEdge < stopEdge; startEdge++) {
			uint32_t k = ind[startEdge];
			// If this is a neighbor and has not been found
			if(level[k] == (level[currElement]-1)){
				delta[k] += (deltadivsigma*(float)sigma[k]);
			}
			
		}
		if(currElement!=currRoot){
			totalBC[currElement]+=delta[currElement];
		}
		leftOver--;
	}
}

void bcDependencyBranchAvoidingSOA(uint32_t currRoot,uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t reverseStart, uint32_t numElements, 
	void* bcSOAStruct, float* totalBC) //	uint32_t* level,uint32_t* sigma, float*delta, float* totalBC)
{
	uint32_t* level = ((bcSOA*)bcSOAStruct)->level;
	uint32_t* sigma = ((bcSOA*)bcSOAStruct)->sigma;
	float* delta = ((bcSOA*)bcSOAStruct)->delta;


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
		int32_t prevLevel = level[currElement]-1;

		float deltadivsigma = (float)(delta[currElement]+1)/(float)(sigma[currElement]);
#if defined(X86)
		__m128 tempstupid;				
		__m128i mmiprevLevel = _mm_cvtsi32_si128(prevLevel);
		__m128 mmDDS        = _mm_load_ss (&deltadivsigma);
#endif

		for (uint32_t j = startEdge; startEdge < stopEdge; startEdge++) {
			uint32_t k = ind[startEdge];	
#if defined(X86)

			__m128i mmiiLevelk   = _mm_cvtsi32_si128(level[k]);		
			__m128 mmsigmak      = _mm_cvtsi32_ss(tempstupid,sigma[k]);
			__m128 mmdeltak      = _mm_load_ss  (delta+k);
	
			__m128i mmiCmpEq     = _mm_cmpeq_epi32 (mmiprevLevel, mmiiLevelk);
			mmsigmak             = _mm_and_si128(mmsigmak,mmiCmpEq);
			mmdeltak             = _mm_fmadd_ss (mmDDS,mmsigmak,mmdeltak);
		   _mm_store_ss(delta+k, mmdeltak);

#endif

#if defined(ARMASM)
			int levelk=level[k],sigmak, k4=k<<2;
     		float sigmakf,deltak;//=delta[k];
			int deltakpos,sigmakpos;
			__asm__ __volatile__ ( ""
				"CMP %[prevLevel], %[levelk]          	   ;\n\t"
				"ADDEQ %[deltakpos], %[delta],%[k4]       ; \n\t"
				"VLDREQ.32 %[deltak], [%[deltakpos],#0]	  ; \n\t"
				"ADDEQ %[sigmakpos], %[sigma],%[k4] ;       \n\t"
				"VLDREQ.32 %[sigmak], [%[sigmakpos],#0]	  ; \n\t"
				"VCVTREQ.F32.S32 %[sigmakf],%[sigmak]; \n\t"
				"VFMAEQ.F32 %[deltak], %[sigmakf], %[deltadivsigma]; \n\t"
				"VSTREQ.32 %[deltak], [%[deltakpos],#0]	  ; \n\t"
 				:
				[deltak] "+w" (deltak) , 
				[sigmakf] "+w" (sigmakf),
				[deltakpos] "+r" (deltakpos),
				[sigmakpos] "+r" (sigmakpos)
				:
				[sigmak] "w" (sigmak),
				[levelk] "r" (levelk), 
				[prevLevel] "r" (prevLevel),
				[delta] "r" (delta), 
				[sigma] "r" (sigma) , 
				[deltadivsigma] "w" (deltadivsigma), 
				[k4] "r" (k4) 
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

void Benchmark_BC(const char* algorithm_name, const char* implementation_name, const struct PerformanceCounter performanceCounters[], size_t performanceCounterCount, BC_TRAV_Function bc_trav_function,  BC_DEP_Function bc_dep_function, uint32_t numVertices, uint32_t* off, uint32_t* ind, uint32_t* edgesTraversed,int isSOA) {
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

	bcSOA bcDataSOA;
	bcDataSOA.level=level;
	bcDataSOA.sigma=sigma;
	bcDataSOA.delta=delta;

	bcAOS* bcDataAOS = (bcAOS*)malloc(numVertices * sizeof(bcAOS));

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
			delta[i] = 0.0;
			queue[i] = 0;
			totalBC[i]=0;
			bcDataAOS[i].level=INT32_MAX;
			bcDataAOS[i].sigma=0;
			bcDataAOS[i].delta=0;
		}

		uint32_t currentLevel = 0;
		uint32_t queuePosition=0;

		level[rootVertex] = currentLevel;
		sigma[rootVertex] = 1;
		queue[0] = rootVertex;

		bcDataAOS[rootVertex].level=currentLevel;
		bcDataAOS[rootVertex].sigma=1;


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
			void* tempPtrSOAAOS=bcDataAOS;
			if (isSOA)
				tempPtrSOAAOS = (void*)&bcDataSOA;
			outputVertices = bc_trav_function(
				off, 
				ind,
				queue,
				queuePosition,
				outputVertices,
				queuePosition+outputVertices,
				tempPtrSOAAOS
				// level,
				// sigma,
				// delta
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
	  		printf("%11"PRIu32, perf_events[levelCount * performanceCounterIndex + level]);
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
			void* tempPtrSOAAOS=bcDataAOS;
			if (isSOA)
				tempPtrSOAAOS = (void*)&bcDataSOA;

			bc_dep_function(
				rootVertex,
				off, 
				ind,
				queue,
				verticesPrefixSum[currentLevel],
				vertices[currentLevel],
				tempPtrSOAAOS,	
				// level,
				// sigma,
				// delta,
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
	  		printf("%11"PRIu32, perf_events[levelCount * performanceCounterIndex + level]);
		}
		printf("\t%"PRIu32"\t%"PRIu32"\n", vertices[level], edgesTraversed[level]);
	}

	free(bcDataAOS);
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


void testBC(BC_TRAV_Function bc_trav_function,  BC_DEP_Function bc_dep_function, uint32_t numVertices, uint32_t* off, uint32_t* ind, uint32_t* level,uint32_t* sigma,float* delta,float* totalBC, bcAOS* bcDataAOS,int isSOA) {
	struct perf_event_attr perf_counter;
	const uint32_t rootVertex = 1;

	uint32_t* queue = (uint32_t*)memalign(64, numVertices * sizeof(uint32_t));
	uint32_t* stack = (uint32_t*)memalign(64, numVertices * sizeof(uint32_t));
	uint32_t* vertices = (uint32_t*)malloc(numVertices * sizeof(uint32_t));
	uint32_t* verticesPrefixSum = (uint32_t*)malloc(numVertices * sizeof(uint32_t));

	bcSOA bcDataSOA;
	bcDataSOA.level=level;
	bcDataSOA.sigma=sigma;
	bcDataSOA.delta=delta;

	void* tempPtrSOAAOS=bcDataAOS;
	if (isSOA)
		tempPtrSOAAOS = (void*)&bcDataSOA;

	int32_t levelCount = 0;
	uint32_t totalVertices = 0;
		/* Initialize level array */
		for (size_t i = 0; i < numVertices; i++) {
			level[i] = INT32_MAX;
			sigma[i] = 0;
			delta[i] = 0;
			queue[i] = 0;
			totalBC[i]=0;
			bcDataAOS[i].level=INT32_MAX;
			bcDataAOS[i].sigma=0;
			bcDataAOS[i].delta=0.0;	
		}

		int32_t currentLevel = 0;
		uint32_t queuePosition=0;

		level[rootVertex] = currentLevel;
		sigma[rootVertex] = 1;
		queue[0] = rootVertex;

		bcDataAOS[rootVertex].level=currentLevel;
		bcDataAOS[rootVertex].sigma=1;

		currentLevel++;

		uint32_t outputVertices = 1;
		do {
			const uint32_t inputVertices = outputVertices;
				vertices[currentLevel-1] = inputVertices;
				totalVertices+=inputVertices;

			outputVertices = bc_trav_function(
				off, 
				ind,
				queue,
				queuePosition,
				outputVertices,
				queuePosition+outputVertices,
				tempPtrSOAAOS
				// level,
				// sigma,
				// delta
			);

			queuePosition += inputVertices;
			currentLevel += 1;
		} while (outputVertices != 0);	


		int sum=vertices[0];
		verticesPrefixSum[0]=vertices[0];	
		for(int i=1; i <levelCount; i++){
			sum+=vertices[i];
			verticesPrefixSum[i]=verticesPrefixSum[i-1]+vertices[i];
		}

		currentLevel=currentLevel-1;
		do {
			bc_dep_function(
				rootVertex,
				off, 
				ind,
				queue,
				verticesPrefixSum[currentLevel],
				vertices[currentLevel],
				tempPtrSOAAOS,
				// level,
				// sigma,
				// delta,
				totalBC
			);
			currentLevel--;
		} while (currentLevel>=0);

	free(queue);
	free(stack);
	free(vertices);
	free(verticesPrefixSum);
}


void compareImplementations(uint32_t numVertices, uint32_t* off, uint32_t* ind, BC_TRAV_Function bb_bc_trav_function,  BC_DEP_Function bb_bc_dep_function, BC_TRAV_Function ba_bc_trav_function,  BC_DEP_Function ba_bc_dep_function, int isSOA){
	uint32_t* BAlevel = (uint32_t*)memalign(64, numVertices * sizeof(uint32_t));
	uint32_t* BAsigma = (uint32_t*)memalign(64, numVertices * sizeof(uint32_t));
	float*    BAdelta = (float*)memalign(64, numVertices * sizeof(float));
	float*    BAtotalBC = (float*)memalign(64, numVertices * sizeof(float));
	uint32_t* BBlevel = (uint32_t*)memalign(64, numVertices * sizeof(uint32_t));
	uint32_t* BBsigma = (uint32_t*)memalign(64, numVertices * sizeof(uint32_t));
	float*    BBdelta = (float*)memalign(64, numVertices * sizeof(float));
	float*    BBtotalBC = (float*)memalign(64, numVertices * sizeof(float));
	bcAOS* BBbcDataAOS = (bcAOS*)malloc(numVertices * sizeof(bcAOS));
	bcAOS* BAbcDataAOS = (bcAOS*)malloc(numVertices * sizeof(bcAOS));

	testBC(bb_bc_trav_function,  bb_bc_dep_function,numVertices, off, ind, BBlevel,BBsigma, BBdelta,BBtotalBC, BBbcDataAOS,isSOA);
	testBC(ba_bc_trav_function,  ba_bc_dep_function,numVertices, off, ind, BAlevel,BAsigma, BAdelta,BAtotalBC, BAbcDataAOS,isSOA);

//	printf("Starting testing\n");

	if (isSOA){
		for (int i=0; i<numVertices; i++){
			if (BAlevel[i] != BBlevel[i]){
				printf("Levels are not the same\n");
				break;
			}
		}
		for (int i=0; i<numVertices; i++){
			if (BAsigma[i] != BBsigma[i]){
				printf("Sigmas are not the same\n");
				break;
			}
		}
		for (int i=0; i<numVertices; i++){
			if (fabsf(BAdelta[i] - BBdelta[i]) > 0.000000001){
				printf("Deltas are not the same %d\n",i);
				break;
			}
			// if (i<100)
//			printf("(%d, %d),  ", BAsigma[i], BBsigma[i]);
			// printf("(%f, %f),  ", BBdelta[i],BAdelta[i]);
		}
	}
	else{
		printf("\n\n\n");
		for (int i=0; i<numVertices; i++){
			if (BBbcDataAOS[i].level != BAbcDataAOS[i].level){
				printf("Levels are not the same\n");
				break;
			}
		}
		for (int i=0; i<numVertices; i++){
			if (BBbcDataAOS[i].sigma != BAbcDataAOS[i].sigma){
				printf("Sigmas are not the same\n");
				break;
			}
		}
		for (int i=0; i<numVertices; i++){
			if (fabsf(BBbcDataAOS[i].delta - BAbcDataAOS[i].delta) > 0.000000001){
				printf("Deltas are not the same %d\n",i);
			printf("(%f, %f),  ", BBbcDataAOS[i].delta, BAbcDataAOS[i].delta);

				break;
			}
			// if (i<100)
			// printf("(%f, %f),  ", BBbcDataAOS[i].delta, BAbcDataAOS[i].delta);
//			printf("(%d, %d),  ", BBbcDataAOS[i].sigma, BAbcDataAOS[i].sigma);
		}

	}
//	printf("Stopping testing\n");

	free(BBbcDataAOS);	
	free(BAbcDataAOS);	
	free(BBlevel);
	free(BBsigma);
	free(BBdelta);
	free(BBtotalBC);	
	free(BAlevel);
	free(BAsigma);
	free(BAdelta);
	free(BAtotalBC);
}


#endif






uint32_t bcTreeBranchBasedAOS(uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t inputStart, uint32_t inputNum, 
		uint32_t outputStart, void* bcAOSStruct)//uint32_t* level,uint32_t* sigma, float*delta);
{
	bcAOS* aos = (bcAOS*)bcAOSStruct;

	uint32_t* outputQueue=queue+outputStart;
	int32_t qPrevStart=inputStart,qPrevEnd=inputStart+inputNum;
	int32_t qOut=0;
	int32_t k;
	// While queue is not empty
	while(qPrevStart!=qPrevEnd)	{
		uint32_t currElement = queue[qPrevStart++];
		uint32_t nextLevel = aos[currElement].level+1;

		uint32_t startEdge = off[currElement];
		uint32_t stopEdge = off[currElement+1];
		for (uint32_t j = startEdge; startEdge < stopEdge; startEdge++) {
			uint32_t k = ind[startEdge];
			// Checking if "k" has been found.
			if(aos[k].level==INT32_MAX){
				aos[k].level = nextLevel;
				outputQueue[qOut++] = k;
				aos[k].delta=0;
			}
			if(aos[k].level==(nextLevel))
				aos[k].sigma += aos[currElement].sigma;
		}
	}
	return 	qOut;
}


uint32_t bcTreeBranchAvoidingAOS(uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t inputStart, uint32_t inputNum, 
		uint32_t outputStart, void* bcAOSStruct)//uint32_t* level,uint32_t* sigma, float*delta);
{
	bcAOS* aos = (bcAOS*)bcAOSStruct;

	uint32_t* outputQueue=queue+outputStart;
	int32_t qPrevStart=inputStart,qPrevEnd=inputStart+inputNum;
	int32_t qOut=0;
	int32_t k;
	// While queue is not empty
	while(qPrevStart!=qPrevEnd)	{
		uint32_t currElement = queue[qPrevStart++];
		int32_t nextLevel = aos[currElement].level+1;

		uint32_t startEdge = off[currElement];
		uint32_t stopEdge = off[currElement+1];

		for (uint32_t j = startEdge; startEdge < stopEdge; startEdge++) {
			uint32_t k = ind[startEdge];
			// Checking if "k" has been found.
			if(aos[k].level==INT32_MAX){
				aos[k].level = nextLevel;
				outputQueue[qOut++] = k;
				aos[k].delta=0;
			}

#if defined(X86)
            int32_t levelk=aos[k].level;
			int32_t sigmaCurr = aos[currElement].sigma;
			int32_t tempVal=0;
   		__asm__ __volatile__ (
				"CMP %[levelk], %[nextLevel];      \n\t"
				"CMOVE %[sigmaCurr], %[tempVal];   \n\t"
		    	: [tempVal] "+r" (tempVal)
				: [levelk] "r" (levelk), 
				  [nextLevel] "r" (nextLevel),
				  [sigmaCurr] "r" (sigmaCurr)
			);
		   aos[k].sigma+=tempVal;
		
#endif

#if defined(ARMASM)
			int sigmak;//=aos[k].sigma;
			int sigmacurr;//=aos[currElement].sigma;
			int levelk=aos[k].level;
			__asm__ __volatile__ (
				"CMP %[nextLevel], %[levelk];                \n\t"
				"LDREQ %[sigmak], [%[sigma], %[k], LSL #2];  \n\t"
				"LDREQ %[sigmacurr], [%[sigma], %[currElement], LSL #2];  \n\t"
				"ADDEQ %[sigmak],%[sigmak], %[sigmacurr];              \n\t"
				"STREQ %[sigmak], [%[sigma], %[k], LSL #2];  \n\t"
				: 
				[sigmak] "+r" (sigmak),
				[sigmacurr] "+r" (sigmacurr)
				: 
				[levelk] "r" (levelk), 
				[nextLevel] "r" (nextLevel),				
				[sigma] "r" (sigma),
				[currElement] "r" (currElement),
				[k] "r" (k)
			);
#endif
		}
	}
	return 	qOut;
}


void bcDependencyBranchBasedAOS(uint32_t currRoot,uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t reverseStart, uint32_t numElements, 
void* bcAOSStruct, float* totalBC) //	uint32_t* level,uint32_t* sigma, float*delta, float* totalBC)
{
	bcAOS* aos = (bcAOS*)bcAOSStruct;

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
		uint32_t prevLevel = aos[currElement].level-1;
		float deltadivsigma = (float)(aos[currElement].delta+1)/(float)(aos[currElement].sigma);
		
		for (uint32_t j = startEdge; startEdge < stopEdge; startEdge++) {
			uint32_t k = ind[startEdge];
			// If this is a neighbor and has not been found
			if(aos[k].level == (aos[currElement].level-1)){
				aos[k].delta += (deltadivsigma*(float)aos[k].sigma);
			}
			
		}
		if(currElement!=currRoot){
			totalBC[currElement]+=aos[currElement].delta;
		}
		leftOver--;
	}
}

void bcDependencyBranchAvoidingAOS(uint32_t currRoot,uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t reverseStart, uint32_t numElements, 
	void* bcAOSStruct, float* totalBC) //	uint32_t* level,uint32_t* sigma, float*delta, float* totalBC)
{
	bcAOS* aos = (bcAOS*)bcAOSStruct;

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
		int32_t prevLevel = aos[currElement].level-1;

		float deltadivsigma = (float)(aos[currElement].delta+1)/(float)(aos[currElement].sigma);
#if defined(X86)
		__m128 tempstupid;				
		__m128i mmiprevLevel = _mm_cvtsi32_si128(prevLevel);
		__m128 mmDDS        = _mm_load_ss (&deltadivsigma);
#endif

		for (uint32_t j = startEdge; startEdge < stopEdge; startEdge++) {
			uint32_t k = ind[startEdge];	
#if defined(X86)
			float temp=aos[k].delta;

			__m128i mmiiLevelk   = _mm_cvtsi32_si128(aos[k].level);		
			__m128 mmsigmak      = _mm_cvtsi32_ss(tempstupid,aos[k].sigma);
			__m128 mmdeltak      = _mm_load_ss  (&temp);
	
			__m128i mmiCmpEq     = _mm_cmpeq_epi32 (mmiprevLevel, mmiiLevelk);
			mmsigmak             = _mm_and_si128(mmsigmak,mmiCmpEq);
			mmdeltak             = _mm_fmadd_ss (mmDDS,mmsigmak,mmdeltak);
		   _mm_store_ss(&temp, mmdeltak);
			aos[k].delta=temp;	

#endif

#if defined(ARMASM)
			int levelk=aos[k].level,sigmak, k4=k<<2;
     		float sigmakf,deltak;//=aos[k].delta;
			int deltakpos,sigmakpos;
			__asm__ __volatile__ ( ""
				"CMP %[prevLevel], %[levelk]          	   ;\n\t"
				"ADDEQ %[deltakpos], %[delta],%[k4]       ; \n\t"
				"VLDREQ.32 %[deltak], [%[deltakpos],#0]	  ; \n\t"
				"ADDEQ %[sigmakpos], %[sigma],%[k4] ;       \n\t"
				"VLDREQ.32 %[sigmak], [%[sigmakpos],#0]	  ; \n\t"
				"VCVTREQ.F32.S32 %[sigmakf],%[sigmak]; \n\t"
				"VFMAEQ.F32 %[deltak], %[sigmakf], %[deltadivsigma]; \n\t"
				"VSTREQ.32 %[deltak], [%[deltakpos],#0]	  ; \n\t"
 				:
				[deltak] "+w" (deltak) , 
				[sigmakf] "+w" (sigmakf),
				[deltakpos] "+r" (deltakpos),
				[sigmakpos] "+r" (sigmakpos)
				:
				[sigmak] "w" (sigmak),
				[levelk] "r" (levelk), 
				[prevLevel] "r" (prevLevel),
				[delta] "r" (delta), 
				[sigma] "r" (sigma) , 
				[deltadivsigma] "w" (deltadivsigma), 
				[k4] "r" (k4) 
			);
#endif
		}
		if(currElement!=currRoot){
			totalBC[currElement]+=aos[currElement].delta;
		}
		leftOver--;
	}

}

