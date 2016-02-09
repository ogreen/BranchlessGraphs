#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <inttypes.h>
#include <assert.h>
#include <time.h>
#include <math.h>

#if defined(__x86_64__)
	#include <x86intrin.h>
#endif

#include "main.h"

#if defined(BENCHMARK_SV)
void Benchmark_ConnectedComponents_SV(const char* algorithm_name, const char* implementation_name, const struct PerformanceCounter performanceCounters[], size_t performanceCounterCount, ConnectedComponents_SV_Function* sv_function, eSV_Alg* algPerIteration,size_t numVertices, size_t numEdges, uint32_t* off, uint32_t* ind){
  struct perf_event_attr perf_counter;

  uint32_t* components_map = (uint32_t*)memalign(64, numVertices * sizeof(uint32_t));
  uint64_t* perf_events = (uint64_t*)malloc(numVertices * sizeof(uint64_t));
  uint32_t* vertices = (uint32_t*)malloc(numVertices * sizeof(uint32_t));

  uint32_t iterationCount = 0;
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
	  components_map[i] = i;
	}

	bool changed;
	size_t iteration = 0;
	do {
	  struct timespec startTime;
	  if (performanceCounters[performanceCounterIndex].type == PERF_TYPE_TIME) {
		assert(clock_gettime(CLOCK_MONOTONIC, &startTime) == 0);
	  } else {
		assert(ioctl(perf_counter_fd, PERF_EVENT_IOC_RESET, 0) == 0);
		assert(ioctl(perf_counter_fd, PERF_EVENT_IOC_ENABLE, 0) == 0);
	  }

	  //printf("@@ %d @@ ",algPerIteration[iteration]);
	  changed = (sv_function[algPerIteration[iteration]])(numVertices, components_map, off, ind);

	  if (performanceCounters[performanceCounterIndex].type == PERF_TYPE_TIME) {
		struct timespec endTime;
		assert(clock_gettime(CLOCK_MONOTONIC, &endTime) == 0);
		perf_events[iterationCount * performanceCounterIndex + iteration] =
		  (1000000000ll * endTime.tv_sec + endTime.tv_nsec) - 
		  (1000000000ll * startTime.tv_sec + startTime.tv_nsec);
	  } else {
		assert(ioctl(perf_counter_fd, PERF_EVENT_IOC_DISABLE, 0) == 0);
		assert(read(perf_counter_fd, &perf_events[iterationCount * performanceCounterIndex + iteration], sizeof(uint64_t)) == sizeof(uint64_t));
	  }
	  iteration += 1;
	} while (changed);
	if (iterationCount == 0) {
	  iterationCount = iteration;
	  perf_events = realloc(perf_events, numVertices * sizeof(uint64_t) * iterationCount);
	}
	close(perf_counter_fd);
  }
  for (uint32_t iteration = 0; iteration < iterationCount; iteration++) {
	printf("%s\t%s\t%"PRIu32, algorithm_name, implementation_name, iteration);
	for (size_t performanceCounterIndex = 0; performanceCounterIndex < performanceCounterCount; performanceCounterIndex++) {
	  if (!performanceCounters[performanceCounterIndex].supported)
		continue;
	  printf("\t%"PRIu64, perf_events[iterationCount * performanceCounterIndex + iteration]);
	}
	printf("\t%zu\t%zu\n", numVertices, numEdges);
  }
  free(components_map);
  free(perf_events);
  free(vertices);
}

void swapAlg(eSV_Alg* alg )
{ 
  if(*alg==SV_ALG_BRANCH_AVOIDING){
	*alg=SV_ALG_BRANCH_BASED;
  }
  else if(*alg==SV_ALG_BRANCH_BASED){
	*alg=SV_ALG_BRANCH_AVOIDING;
  }

}

void resetLastSwap(int32_t* lastSwap){
  *lastSwap=0;
}


void ConnectedComponentsSVHybridIterationSelector(const char* algorithm_name, const char* implementation_name, const struct PerformanceCounter performanceCounters[], size_t performanceCounterCount, ConnectedComponents_SV_Function* sv_function, eSV_Alg* algPerIteration,svControlParams svCP,size_t numVertices, size_t numEdges, uint32_t* off, uint32_t* ind){
  struct perf_event_attr perf_counter;

  uint32_t* components_map = (uint32_t*)memalign(64, numVertices * sizeof(uint32_t));
  uint32_t* vertices = (uint32_t*)malloc(numVertices * sizeof(uint32_t));

  //        uint32_t iterationCount = 0;
  for (size_t i = 0; i < numVertices; i++) {
	components_map[i] = i;
  }

  bool changed=true;
  size_t iteration = 0;
  double time_curr=0.0,time_prev=0.0;
  algPerIteration[iteration]=SV_ALG_BRANCH_AVOIDING;
  int32_t lastSwap;resetLastSwap(&lastSwap);

  // First iteration will be branch-avoiding.
  algPerIteration[iteration]=SV_ALG_BRANCH_AVOIDING;
  tic();
  changed = (sv_function[algPerIteration[iteration]])(numVertices, components_map, off, ind);
  time_prev=toc();
  //   		printf("%d ,\n",algPerIteration[iteration]);
  // Second iteration will be branc-based(assuming that it is needed)
  iteration++;
  if(changed){
	algPerIteration[iteration]=SV_ALG_BRANCH_BASED;
	tic();
	changed = (sv_function[algPerIteration[iteration]])(numVertices, components_map, off, ind);
	time_curr=toc();
  }
  //   		printf("%d ,\n",algPerIteration[iteration]);
  resetLastSwap(&lastSwap);
  while (changed)
  {
	iteration++;
	lastSwap++;
	//int32_t cond=(fabs(time_curr-time_prev)>0.1*time_curr);
	//printf("%lf %lf %lf %lf ",time_curr,time_prev,fabs(time_curr-time_prev),0.1*time_curr);
	int32_t gradientDetectionCond=((time_curr-time_prev)>0.05*time_curr); 
	int32_t steadyStateCond=(fabs(time_curr-time_prev)>0.01*time_curr) && lastSwap==1;
	//			  printf("%lf %lf %lf %lf ",time_curr,time_prev,(time_curr-time_prev),0.1*time_curr);
	algPerIteration[iteration]=algPerIteration[iteration-1];

	int32_t swapCond=gradientDetectionCond || steadyStateCond;
	if(swapCond){
	  algPerIteration[iteration]=SV_ALG_BRANCH_AVOIDING;
	  resetLastSwap(&lastSwap);
	  //				  printf("*");
	}
	// Checking if the algorithm has been stuck for more than 5 iterations using the same algorithm.
	if(lastSwap==5){
	  swapAlg(algPerIteration+iteration);
	  resetLastSwap(&lastSwap);
	}

	// Swapping times with the last iteration and starting and additional iteration.
	time_prev=time_curr;
	tic();
	changed = (sv_function[algPerIteration[iteration]])(numVertices, components_map, off, ind);
	time_curr=toc();

  }

  free(components_map);
  free(vertices);
}


#endif



#ifdef __SSE4_1__
	bool ConnectedComponents_SV_Branchless_SSE4_1(size_t vertexCount, uint32_t* componentMap, uint32_t* vertexEdges, uint32_t* neighbors) {
		uint32_t changed = 0;

		for (size_t vertex=0; vertex < vertexCount; vertex++) {
			const uint32_t *restrict neighborPointer = &neighbors[vertexEdges[vertex]];
			const size_t vdeg = vertexEdges[vertex + 1] - vertexEdges[vertex];
			const uint32_t currentComponent = componentMap[vertex];
			uint32_t component_v_new = currentComponent;
			size_t edge = 0;
			if (vdeg >= 4) {
				__m128i vec_component_v_new = _mm_set1_epi32(component_v_new);
				for (; edge < (vdeg & -4); edge += 4){
					const uint32_t u0 = neighborPointer[edge];
					const uint32_t u1 = neighborPointer[edge + 1];
					const uint32_t u2 = neighborPointer[edge + 2];
					const uint32_t u3 = neighborPointer[edge + 3];
					const __m128i vec_component_u = _mm_insert_epi32(_mm_insert_epi32(_mm_insert_epi32(_mm_cvtsi32_si128(componentMap[u0]), componentMap[u1], 1), componentMap[u2], 2), componentMap[u3], 3);
					vec_component_v_new = _mm_min_epu32(vec_component_u, vec_component_v_new);
				}
				vec_component_v_new = _mm_min_epu32(vec_component_v_new, _mm_shuffle_epi32(vec_component_v_new, _MM_SHUFFLE(3, 2, 3, 2)));
				vec_component_v_new = _mm_min_epu32(vec_component_v_new, _mm_shuffle_epi32(vec_component_v_new, _MM_SHUFFLE(3, 2, 1, 1)));
				component_v_new = _mm_cvtsi128_si32(vec_component_v_new);
			}
			for (; edge < vdeg; edge += 1){
				const uint32_t neighborVertex = neighborPointer[edge];
				const uint32_t component_u = componentMap[neighborVertex];
				if (component_u < component_v_new) {
					component_v_new = component_u;
				}
			}
			changed |= currentComponent ^ component_v_new;
			componentMap[vertex] = component_v_new;
		}

		if (!changed)
			return false;

		//~ for (size_t i = 0; i < vertexCount; i++) {
			//~ while (componentMap[i] != componentMap[componentMap[i]]) {
				//~ componentMap[i] = componentMap[componentMap[i]];
			//~ }
		//~ }

		return true;
	}
#endif

#ifdef __MIC__
	bool ConnectedComponents_SV_Branchless_MIC(size_t vertexCount, uint32_t* componentMap, uint32_t* vertexEdges, uint32_t* neighbors) {
		__mmask16 changed = 0;

		for (size_t vertex=0; vertex < vertexCount; vertex++) {
			const uint32_t *restrict neighborPointer = &neighbors[vertexEdges[vertex]];
			const size_t vdeg = vertexEdges[vertex + 1] - vertexEdges[vertex];
			if (vdeg != 0) {
				const __m512i currentComponent = _mm512_extload_epi32(&componentMap[vertex], _MM_UPCONV_EPI32_NONE, _MM_BROADCAST_1X16, _MM_HINT_NONE);
				__m512i component_v_new = currentComponent;
				if (vdeg >= 16) {
					size_t edge = 0;
					for (; edge + 15 < vdeg; edge += 16){
						const __m512i neighborVertex = _mm512_loadunpackhi_epi32(_mm512_loadunpacklo_epi32(_mm512_undefined_epi32(), &neighborPointer[edge]), &neighborPointer[edge + 16]);
						const __m512i component_u = _mm512_i32gather_epi32(neighborVertex, componentMap, sizeof(uint32_t));
						component_v_new = _mm512_min_epu32(component_v_new, component_u);
					}
					const size_t remainder = vdeg - edge;
					if (remainder != 0) {
						__mmask16 mask = _mm512_int2mask((1 << remainder) - 1);
						const __m512i neighborVertex = _mm512_mask_loadunpackhi_epi32(_mm512_mask_loadunpacklo_epi32(_mm512_undefined_epi32(), mask, &neighborPointer[edge]), mask, &neighborPointer[edge + 16]);
						const __m512i component_u = _mm512_mask_i32gather_epi32(_mm512_undefined_epi32(), mask, neighborVertex, componentMap, sizeof(uint32_t));
						component_v_new = _mm512_mask_min_epu32(component_v_new, mask, component_v_new, component_u);
					}
					changed = _mm512_kor(changed, _mm512_cmp_epu32_mask(currentComponent, component_v_new, _MM_CMPINT_NE));
					componentMap[vertex] = _mm512_reduce_min_epu32(component_v_new);
				} else {
					__mmask16 mask = _mm512_int2mask((1 << vdeg) - 1);
					const __m512i neighborVertex = _mm512_mask_loadunpackhi_epi32(_mm512_mask_loadunpacklo_epi32(_mm512_undefined_epi32(), mask, &neighborPointer[0]), mask, &neighborPointer[16]);
					const __m512i component_u = _mm512_mask_i32gather_epi32(_mm512_undefined_epi32(), mask, neighborVertex, componentMap, sizeof(uint32_t));
					component_v_new = _mm512_mask_min_epu32(component_v_new, mask, component_v_new, component_u);
					changed = _mm512_kor(changed, _mm512_cmp_epu32_mask(currentComponent, component_v_new, _MM_CMPINT_NE));
					componentMap[vertex] = _mm512_mask_reduce_min_epu32(mask, component_v_new);
				}
			}
		}

		if (_mm512_kortestz(changed, changed))
			return false;

		//~ for (size_t i = 0; i < vertexCount; i++) {
			//~ while (componentMap[i] != componentMap[componentMap[i]]) {
				//~ componentMap[i] = componentMap[componentMap[i]];
			//~ }
		//~ }

		return true;
	}
#endif
