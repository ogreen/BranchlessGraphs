#include "main.h"
#include "timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <inttypes.h>
#include <assert.h>
#include <time.h>
#include <math.h>

#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>

#include "cct.h"

#define PERF_TYPE_TIME PERF_TYPE_MAX

struct PerformanceCounter {
    const char* name;
    uint32_t type;
    uint32_t subtype;
    bool supported;
};

typedef enum{
  SV_ALG_BRANCH_BASED=0,
  SV_ALG_BRANCH_AVOIDING,
  SV_NUMBER_OF_ALGS
  } eSV_Alg;

void CheckPerformanceCounters(struct PerformanceCounter performanceCounters[], size_t performanceCountersCount);

void Benchmark_BFS_TopDown(const char* algorithm_name, const char* implementation_name,
    const struct PerformanceCounter performanceCounters[], size_t performanceCountersCount,
    BFS_TopDown_Function bfs_function, uint32_t numVertices, uint32_t* off, uint32_t* ind, uint32_t* edgesTraversed);
void Benchmark_BFS_BottomUp(const char* algorithm_name, const char* implementation_name, BFS_BottomUp_Function bfs_function, uint32_t numVertices, uint32_t* off, uint32_t* ind);
void Benchmark_ConnectedComponents_SV(const char* algorithm_name, const char* implementation_name,
    const struct PerformanceCounter performanceCounters[], size_t performanceCountersCount,
    ConnectedComponents_SV_Function* sv_function, eSV_Alg* algPerIteration, size_t numVertices, size_t numEdges, uint32_t* off, uint32_t* ind);

 typedef struct {
  int32_t iterationSwap;  
  float   toleranceGradient;
  float	  toleranceSteadyState;
} svControlParams;
  
void ConnectedComponentsSVHybridIterationSelector(const char* algorithm_name, const char* implementation_name, 
	const struct PerformanceCounter performanceCounters[], size_t performanceCounterCount, 
	ConnectedComponents_SV_Function* sv_function, eSV_Alg* algPerIteration,svControlParams svCP,size_t numVertices, size_t numEdges, uint32_t* off, uint32_t* ind);

#define LINE_SIZE 10000


static int perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags) {
	return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

void CheckPerformanceCounters(struct PerformanceCounter performanceCounters[], size_t performanceCountersCount) {
    for (size_t performanceCounterIndex = 0; performanceCounterIndex < performanceCountersCount; performanceCounterIndex++) {
        if (performanceCounters[performanceCounterIndex].type == PERF_TYPE_TIME) {
            performanceCounters[performanceCounterIndex].supported = true;
            continue;
        }
        
		struct perf_event_attr perf_counter;
        memset(&perf_counter, 0, sizeof(struct perf_event_attr));
        perf_counter.type = performanceCounters[performanceCounterIndex].type;
        perf_counter.size = sizeof(struct perf_event_attr);
        perf_counter.config = performanceCounters[performanceCounterIndex].subtype;
        perf_counter.disabled = 1;
        perf_counter.exclude_kernel = 1;
        perf_counter.exclude_hv = 1;

        performanceCounters[performanceCounterIndex].supported = true;
        int perf_counter_fd = perf_event_open(&perf_counter, 0, -1, -1, 0);
        if (perf_counter_fd == -1) {
            performanceCounters[performanceCounterIndex].supported = false;
            continue;
        }

        if(ioctl(perf_counter_fd, PERF_EVENT_IOC_RESET, 0) != 0) {
            performanceCounters[performanceCounterIndex].supported = false;
        } else {
            if (ioctl(perf_counter_fd, PERF_EVENT_IOC_ENABLE, 0) != 0) {
                performanceCounters[performanceCounterIndex].supported = false;
            } else {
                if (ioctl(perf_counter_fd, PERF_EVENT_IOC_DISABLE, 0) != 0) {
                    performanceCounters[performanceCounterIndex].supported = false;
                } else {
                    uint64_t dummy;
                    if (read(perf_counter_fd, &dummy, sizeof(uint64_t)) != sizeof(uint64_t)) {
                        performanceCounters[performanceCounterIndex].supported = false;
                    }
                }
            }
            if (close(perf_counter_fd) != 0) {
                performanceCounters[performanceCounterIndex].supported = false;
            }
        }
    }
}

void PrintHeader(const char* precolumns[], const struct PerformanceCounter performanceCounters[], size_t performanceCountersCount, const char* postcolumns[]) {
    bool firstColumn = true;
    while (*precolumns != NULL) {
        if (firstColumn) {
            printf("%s", *precolumns);
            firstColumn = false;
        } else {
            printf("\t%s", *precolumns);
        }
        precolumns++;
    }
    for (size_t performanceCounterIndex = 0; performanceCounterIndex < performanceCountersCount; performanceCounterIndex++) {
        if (!performanceCounters[performanceCounterIndex].supported)
            continue;
        printf("\t%s", performanceCounters[performanceCounterIndex].name);
    }
    while (*postcolumns != NULL) {
        printf("\t%s", *postcolumns);
        postcolumns++;
    }
    printf("\n");
}


#define COUNTOF(array) (sizeof(array) / sizeof(array[0]))

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
	
    struct PerformanceCounter perfCounters[] = {
        { "Time", PERF_TYPE_TIME },
        { "Cycles", PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES},
        { "Instructions", PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS},
    #if defined(HAVE_INTEL_HASWELL_COUNTERS) || defined(HAVE_INTEL_IVYBRIDGE_COUNTERS)
        { "Loads.Retired", PERF_TYPE_RAW, 0x81D0 }, // D0H 01H MEM_UOPS_RETIRED.LOADS
        { "Stores.Retired", PERF_TYPE_RAW, 0x82D0 }, // D0H 01H MEM_UOPS_RETIRED.STORES
        { "Stall.RS", PERF_TYPE_RAW, 0x04A2 }, // A2H 04H RESOURCE_STALLS.RS Cycles stalled due to no eligible RS entry available. 
        { "Stall.SB", PERF_TYPE_RAW, 0x08A2 }, // A2H 08H RESOURCE_STALLS.SB Cycles stalled due to no store buffers available (not including draining form sync).
        { "Stall.ROB", PERF_TYPE_RAW, 0x10A2 }, // A2H 10H RESOURCE_STALLS.ROB
    #endif
    #if defined(HAVE_INTEL_SILVERMONT_COUNTERS)
        { "Stall.ROB", PERF_TYPE_RAW, 0x01CA }, // CAH 01H NO_ALLOC_CYCLES.ROB_FULL Counts the number of cycles when no uops are allocated and the ROB is full (less than 2 entries available)
        { "Stall.RAT", PERF_TYPE_RAW, 0x20CA }, // CAH 01H NO_ALLOC_CYCLES.RAT_STALL Counts the number of cycles when no uops are allocated and a RATstall is asserted. 
        { "Stall.MEC", PERF_TYPE_RAW, 0x01CB }, // CBH 01H RS_FULL_STALL.MEC MEC RS full This event countsthe number of cycles the allocation pipe line stalled due to the RS for the MEC cluster is full
        { "Stall.AnyRS", PERF_TYPE_RAW, 0x1FCB }, // CBH 1FH RS_FULL_STALL.ALL Any RS full This event countsthe number of cycles that the allocation pipe line stalled due to any one of the RS is full
        { "Loads.RehabQ", PERF_TYPE_RAW, 0x4003 }, // 03H 40H REHABQ.ANY_LD Any reissued load uops This event counts the number of loaduops reissued from Rehabq
        { "Stores.RehabQ", PERF_TYPE_RAW, 0x8003 }, // 03H 80H REHABQ.ANY_ST Any reissued store uops This event counts the number of store uops reissued from Rehabq
        { "Loads.Retired", PERF_TYPE_RAW, 0x4004 }, // 04H 40H MEM_UOPS_RETIRED.ALL_LOADS All Loads  This event counts the number of load ops retired 
        { "Stores.Retired", PERF_TYPE_RAW, 0x8004 }, // 04H 80H MEM_UOP_RETIRED.ALL_STORES All Stores  This event counts the number of store ops retired
    #endif
    #if defined(HAVE_AMD_FAMILY15_COUNTERS)
        { "Stall.SB", PERF_TYPE_RAW, 0x0223 }, // The number of cycles that the store buffer is full.
        { "Stall.LB", PERF_TYPE_RAW, 0x0123 }, // The number of cycles that the load buffer is full.
        { "Loads.Dispatched", PERF_TYPE_RAW, 0x0129 },
        { "Stores.Dispatched", PERF_TYPE_RAW, 0x0229 },
        { "Stall.LDQ", PERF_TYPE_RAW, 0x01D8 }, // Dispatch Stall for LDQ Full
    #endif
        { "Cache references", PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_REFERENCES},
        { "Cache misses", PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES},
        { "Branches", PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS},
        { "Mispredictions", PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES}
    };
    
    CheckPerformanceCounters(perfCounters, COUNTOF(perfCounters));

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

        const char* precolumns[] = {
            "Algorithm",
            "Implementation",
            "Iteration",
            NULL
        };
        const char* postcolumns[] = {
            "Vertices",
            "Edges",
            NULL
        };
        PrintHeader(precolumns, perfCounters, COUNTOF(perfCounters), postcolumns);
		Benchmark_BFS_TopDown("BFS/TD", "Branch-based", perfCounters, COUNTOF(perfCounters), BFS_TopDown_Branchy_PeachPy, nv, off, ind, edgesTraversed);
		Benchmark_BFS_TopDown("BFS/TD", "Branch-avoiding", perfCounters, COUNTOF(perfCounters), BFS_TopDown_Branchless_PeachPy, nv, off, ind, edgesTraversed);
		//Benchmark_BFS_TopDown("BFS/TD", "Branch-lessless", BFS_TopDown_Branchlessless_PeachPy, nv, off, ind, edgesTraversed);
		#ifdef __SSE4_1__
		Benchmark_BFS_TopDown("BFS/TD", "Branch-avoiding (SSE 4.1)", BFS_TopDown_Branchless_SSE4_1, nv, off, ind, edgesTraversed);
		#endif
		#ifdef __AVX2__
		Benchmark_BFS_TopDown("BFS/TD", "Branch-avoiding (AVX 2)", BFS_TopDown_Branchless_AVX2, nv, off, ind, edgesTraversed);
		#endif
		#ifdef __MIC__
		Benchmark_BFS_TopDown("BFS/TD", "Branch-avoiding (MIC)", BFS_TopDown_Branchless_MIC, nv, off, ind, edgesTraversed);
		#endif
		//~ Benchmark_BFS_BottomUp("BFS/BU", "Branch-based", BFS_BottomUp_Branchy, nv, off, ind);
		//~ Benchmark_BFS_BottomUp("BFS/BU", "Branch-avoiding (C)", BFS_BottomUp_Branchless, nv, off, ind);
		//~ Benchmark_BFS_BottomUp("BFS/BU", "Branch-avoiding (CMOV)", BFS_BottomUp_Branchless_CMOV, nv, off, ind);

		free(edgesTraversed);
	#endif

#if defined(BENCHMARK_SV)
	  const char* precolumns[] = {
		  "Algorithm",
		  "Implementation",
		  "Itetarion",
		  NULL
	  };
	  const char* postcolumns[] = {
		  "Vertices",
		  "Edges",
		  NULL
	  };


	  ConnectedComponents_SV_Function* sv_function=(ConnectedComponents_SV_Function*) malloc(SV_NUMBER_OF_ALGS*sizeof(ConnectedComponents_SV_Function));
	  sv_function[SV_ALG_BRANCH_BASED]=ConnectedComponents_SV_Branchy_PeachPy;
	  sv_function[SV_ALG_BRANCH_AVOIDING]=ConnectedComponents_SV_Branchless_PeachPy;
      eSV_Alg *iterBB, *iterBA, *iterHybrid;
	  iterBB=(eSV_Alg*)malloc(sizeof(eSV_Alg)*nv);
	  iterBA=(eSV_Alg*)malloc(sizeof(eSV_Alg)*nv);
	  iterHybrid=(eSV_Alg*)malloc(sizeof(eSV_Alg)*nv);
	  
	  for(int v=0; v<nv;v++){
		iterBB[v]=SV_ALG_BRANCH_BASED;
		iterBA[v]=SV_ALG_BRANCH_AVOIDING;
		iterHybrid[v]=SV_ALG_BRANCH_BASED;
	  }

	  PrintHeader(precolumns, perfCounters, COUNTOF(perfCounters), postcolumns);
	  Benchmark_ConnectedComponents_SV("SV", "Branch-based", perfCounters, COUNTOF(perfCounters), sv_function,iterBB, nv, ne, off, ind);
	  Benchmark_ConnectedComponents_SV("SV", "Branch-avoiding", perfCounters, COUNTOF(perfCounters), sv_function,iterBA, nv, ne, off, ind);
	 
	  svControlParams svCP;
	  svCP.toleranceGradient	=0.05;
	  svCP.toleranceSteadyState	=0.01;
	  svCP.iterationSwap		=5;
	  ConnectedComponentsSVHybridIterationSelector ("SV", "Branch-Hybrid", perfCounters, COUNTOF(perfCounters), sv_function,iterHybrid,svCP, nv, ne, off, ind);
	  Benchmark_ConnectedComponents_SV("SV", "Branch-Hybrid", perfCounters, COUNTOF(perfCounters), sv_function,iterHybrid, nv, ne, off, ind);
//	  Benchmark_ConnectedComponents_SV("SV", "Branch-avoiding", perfCounters, COUNTOF(perfCounters), ConnectedComponents_SV_Branchless_PeachPy, nv, ne, off, ind);
//	  Benchmark_ConnectedComponents_SV("SV", "Hybrid", perfCounters, COUNTOF(perfCounters), ConnectedComponents_SV_Branchless_PeachPy, nv, ne, off, ind);
	  #ifdef __SSE4_1__
//	  Benchmark_ConnectedComponents_SV("SV", "SSE4.1", perfCounters, COUNTOF(perfCounters), ConnectedComponents_SV_Branchless_SSE4_1, nv, ne, off, ind);
	  #endif
	  #ifdef __MIC__
//	  Benchmark_ConnectedComponents_SV("SV", "MIC", perfCounters, COUNTOF(perfCounters), ConnectedComponents_SV_Branchless_MIC, nv, ne, off, ind);
	  #endif

      free(iterBB);
	  free(iterBA);
	  free(iterHybrid);

#endif

#if defined(BENCHMARK_CCT)

   	int64_t * triNE = (int64_t *) malloc ((ne ) * sizeof (int64_t));	
	int64_t allTrianglesCPU=0;
	tic();
	triangleCountBranchBased(nv, off,ind, triNE, &allTrianglesCPU);
	double ccbranchbased=toc();

	free(triNE);
#endif

 	free(off);
	free(ind);
}

#if defined(BENCHMARK_BFS)
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
