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


#include "cct.h"

void CheckPerformanceCounters(struct PerformanceCounter performanceCounters[], size_t performanceCountersCount);

//-------------------------------------------------
// BFS
void Benchmark_BFS_TopDown(const char* algorithm_name, const char* implementation_name,
	const struct PerformanceCounter performanceCounters[], size_t performanceCountersCount,
	BFS_TopDown_Function bfs_function, uint32_t numVertices, uint32_t* off, uint32_t* ind, uint32_t* edgesTraversed);
void Benchmark_BFS_TopDown_Trace(const char* algorithm_name, const char* implementation_name,
	const struct PerformanceCounter performanceCounters[], size_t performanceCountersCount,
	BFS_TopDown_Function bfs_function, uint32_t numVertices, uint32_t* off, uint32_t* ind, uint32_t* edgesTraversed, 
	uint32_t* queueStartPosition,uint32_t* level,uint32_t*queue);
void Benchmark_BFS_BottomUp(const char* algorithm_name, const char* implementation_name, BFS_BottomUp_Function bfs_function, uint32_t numVertices, uint32_t* off, uint32_t* ind);

//-------------------------------------------------
// BC
void Benchmark_BC(const char* algorithm_name, const char* implementation_name, const struct PerformanceCounter performanceCounters[], size_t performanceCounterCount, BC_TRAV_Function bc_trav_function,  BC_DEP_Function bc_dep_function, uint32_t numVertices, uint32_t* off, uint32_t* ind, uint32_t* edgesTraversed);


//-------------------------------------------------
// Connected Components
void Benchmark_ConnectedComponents_SV(const char* algorithm_name, const char* implementation_name,
	const struct PerformanceCounter performanceCounters[], size_t performanceCountersCount,
	ConnectedComponents_SV_Function* sv_function, eSV_Alg* algPerIteration, size_t numVertices, size_t numEdges, uint32_t* off, uint32_t* ind);

void ConnectedComponentsSVHybridIterationSelector(const char* algorithm_name, const char* implementation_name, 
	const struct PerformanceCounter performanceCounters[], size_t performanceCounterCount, 
	ConnectedComponents_SV_Function* sv_function, eSV_Alg* algPerIteration,svControlParams svCP,size_t numVertices, size_t numEdges, uint32_t* off, uint32_t* ind);

//-------------------------------------------------
// Triangle Counting
void Benchmark_TriangleCounting(const char* algorithm_name, const char* implementation_name, const struct PerformanceCounter performanceCounters[], size_t performanceCounterCount, CCT_Function cct_function, uint32_t numVertices, uint32_t numEdges,uint32_t* off, uint32_t* ind);



#define LINE_SIZE 10000


int perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags) {
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
  uint32_t * off = (uint32_t *) malloc ((nv + 2) * sizeof (uint32_t));
  uint32_t * ind = (uint32_t *) malloc ((ne * 2) * sizeof (uint32_t));
  off[0] = 0;
  off[1] = 0;
  int32_t counter = 0;
  int32_t u;
  line=NULL;
  bytesRead=0;

  //	  for (u = 1; fgets (line, &bytesRead, fp); u++)
  for (u = 1; (temp=getline (&line, &bytesRead, fp))!=-1; u++)
  {	
	//		printf("%s",line);	,
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
  //	if (argc > 3) {
  //		fprintf(stderr, "Usage: bfs <graph-name>\n");
  //		exit(EXIT_FAILURE);
  //	}

  uint32_t nv, ne, naction;
  uint32_t* off;
  uint32_t* ind;
  readGraphDIMACS(argv[1], &off, &ind, &nv, &ne);

  struct PerformanceCounter perfCounters[] = {
	{ "Time", PERF_TYPE_TIME },
	{ "Cycles", PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES},
	{ "Instructions", PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS},
#if defined(HAVE_INTEL_BROADWELL_COUNTERS)|| defined(HAVE_INTEL_HASWELL_COUNTERS) || defined(HAVE_INTEL_IVYBRIDGE_COUNTERS)
	{ "Loads.Retired", PERF_TYPE_RAW, 0x81D0 }, // D0H 01H MEM_UOPS_RETIRED.LOADS
	{ "Stores.Retired", PERF_TYPE_RAW, 0x82D0 }, // D0H 01H MEM_UOPS_RETIRED.STORES
//	{ "Stall.LB", PERF_TYPE_RAW, 0x02A2 }, // A2H 02H RESOURCE_STALLS.LB Cycles Allocator is stalled due to Load Buffer full .
	{ "Stall.RS", PERF_TYPE_RAW, 0x04A2 }, // A2H 04H RESOURCE_STALLS.RS Cycles stalled due to no eligible RS entry available. 
//	{ "Stall.SB", PERF_TYPE_RAW, 0x08A2 }, // A2H 08H RESOURCE_STALLS.SB Cycles stalled due to no store buffers available (not including draining form sync).
//	{ "Stall.ROB", PERF_TYPE_RAW, 0x10A2 }, // A2H 10H RESOURCE_STALLS.ROB
	{ "Stall.ooo_rsrc", PERF_TYPE_RAW, 0x0EA2 }, // A2H F0H Resource stalls due to Rob being full, FCSW, MXCSR and OTHER 
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
#if defined(HAVE_ARM_COUNTERS)
#endif
	{ "READS", PERF_TYPE_HW_CACHE, PERF_COUNT_HW_CACHE_LL }, // The number of cycles that the store buffer is full.
	{ "STALLS", PERF_TYPE_HARDWARE, PERF_COUNT_HW_BUS_CYCLES},
	{ "Cache references", PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_REFERENCES},
	{ "Cache misses", PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES},
	{ "Branches", PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS},
	{ "Mispredictions", PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES}
  };
  CheckPerformanceCounters(perfCounters, COUNTOF(perfCounters));

#if defined(BENCHMARK_BFS)	

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

  uint32_t* edgesTraversed = (uint32_t*)memalign(64, nv * sizeof(uint32_t));
  memset(edgesTraversed, 0, nv * sizeof(uint32_t));
  {
	uint32_t* queue = (uint32_t*)memalign(64, nv * sizeof(uint32_t));
	uint32_t* level = (uint32_t*)memalign(64, nv * sizeof(uint32_t));
	uint32_t* queueStartPosition = (uint32_t*)memalign(64, nv * sizeof(uint32_t));
	for (size_t i = 0; i < nv; i++) {
	  level[i] = INT32_MAX;
	  queueStartPosition[i]=INT32_MAX;
	}
	BFS_TopDown_Branchy_LevelInformation(off, ind, queue, level, 1, edgesTraversed, queueStartPosition);


	free(queueStartPosition);
	free(level);
	free(queue);
  }

  Benchmark_BFS_TopDown("BFS/TD", "Branch-based     ", perfCounters, COUNTOF(perfCounters), BFS_TopDown_Branchy_PeachPy, nv, off, ind, edgesTraversed);
  Benchmark_BFS_TopDown("BFS/TD", "Branch-avoiding  ", perfCounters, COUNTOF(perfCounters), BFS_TopDown_Branchless_PeachPy, nv, off, ind, edgesTraversed);
  
   //Benchmark_BFS_TopDown("BFS/TD", "Branch-lessless", BFS_TopDown_Branchlessless_PeachPy, nv, off, ind, edgesTraversed);
// #ifdef __SSE4_1__
//   Benchmark_BFS_TopDown("BFS/TD", "Branch-avoiding (SSE 4.1)", BFS_TopDown_Branchless_SSE4_1, nv, off, ind, edgesTraversed);
// #endif
// #ifdef __AVX2__
//   Benchmark_BFS_TopDown("BFS/TD", "Branch-avoiding (AVX 2)", BFS_TopDown_Branchless_AVX2, nv, off, ind, edgesTraversed);
// #endif
// #ifdef __MIC__
//   Benchmark_BFS_TopDown("BFS/TD", "Branch-avoiding (MIC)", BFS_TopDown_Branchless_MIC, nv, off, ind, edgesTraversed);
// #endif
  //~ Benchmark_BFS_BottomUp("BFS/BU", "Branch-based", BFS_BottomUp_Branchy, nv, off, ind);
  //~ Benchmark_BFS_BottomUp("BFS/BU", "Branch-avoiding (C)", BFS_BottomUp_Branchless, nv, off, ind);
  //~ Benchmark_BFS_BottomUp("BFS/BU", "Branch-avoiding (CMOV)", BFS_BottomUp_Branchless_CMOV, nv, off, ind);
  memset(edgesTraversed, 0, nv * sizeof(uint32_t));
  {
	uint32_t* queue = (uint32_t*)memalign(64, nv * sizeof(uint32_t));
	uint32_t* level = (uint32_t*)memalign(64, nv * sizeof(uint32_t));
	uint32_t* queueStartPosition = (uint32_t*)memalign(64, nv * sizeof(uint32_t));
	for (size_t i = 0; i < nv; i++) {
	  level[i] = INT32_MAX;
	  queueStartPosition[i]=INT32_MAX;
	}
	BFS_TopDown_Branchy_LevelInformation(off, ind, queue, level, 1, edgesTraversed, queueStartPosition);

	Benchmark_BFS_TopDown_Trace("BFS/TD", "Branch-avd-Trace", perfCounters, COUNTOF(perfCounters), BFS_TopDown_Branchless_Trace_PeachPy, nv, off, ind, edgesTraversed,queueStartPosition,level,queue);


	free(queueStartPosition);
	free(level);
	free(queue);
  }



  free(edgesTraversed);
#endif

#if defined(BENCHMARK_BC)	

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

  uint32_t* edgesTraversed = (uint32_t*)memalign(64, nv * sizeof(uint32_t));
  memset(edgesTraversed, 0, nv * sizeof(uint32_t));
  {
	uint32_t* queue = (uint32_t*)memalign(64, nv * sizeof(uint32_t));
	uint32_t* level = (uint32_t*)memalign(64, nv * sizeof(uint32_t));
	uint32_t* queueStartPosition = (uint32_t*)memalign(64, nv * sizeof(uint32_t));
	for (size_t i = 0; i < nv; i++) {
	  level[i] = INT32_MAX;
	  queueStartPosition[i]=INT32_MAX;
	}
	BFS_TopDown_Branchy_LevelInformation(off, ind, queue, level, 1, edgesTraversed, queueStartPosition);

	free(queueStartPosition);
	free(level);
	free(queue);
  }
	Benchmark_BC("BC", "Branch-based     ", perfCounters, COUNTOF(perfCounters), bcTreeBranchBased, 	bcDependencyBranchBased,   nv, off, ind, edgesTraversed);
	Benchmark_BC("BC", "Branch-avoiding  ", perfCounters, COUNTOF(perfCounters), bcTreeBranchAvoiding,	bcDependencyBranchAvoiding, nv, off, ind, edgesTraversed);

void compareImplementations(uint32_t numVertices, uint32_t* off, uint32_t* ind, BC_TRAV_Function bb_bc_trav_function,  BC_DEP_Function bb_bc_dep_function, BC_TRAV_Function ba_bc_trav_function,  BC_DEP_Function ba_bc_dep_function );
compareImplementations(nv, off, ind, bcTreeBranchBased,  bcDependencyBranchBased, bcTreeBranchAvoiding,  bcDependencyBranchAvoiding );	
	
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

  if(1){
	int32_t * triNE = (int32_t *) malloc ((ne ) * sizeof (int32_t));	
	int32_t allTrianglesCPU=0;
	//	tic();

	if (atoi(argv[3])){
	  benchMarkAllSynthetic(nv, ne, off,ind,triNE,atoi(argv[4]),argv[2]);
	}
	else{
	  benchMarkCCT(nv, ne, off,ind, triNE, &allTrianglesCPU,argv[2],0,0);

	}

	free(triNE);
  }
  else{ 
	const char* precolumns[] = {
	  "Algorithm",
	  "Implementation",
	  "Vertices",
	  "Edge",
	  NULL
	};
	const char* postcolumns[] = {
	  "Triangles",
	  "Intersection-Operations",
	  NULL
	};

	PrintHeader(precolumns, perfCounters, COUNTOF(perfCounters), postcolumns);
	Benchmark_TriangleCounting("CCT", "Branch-based", perfCounters, COUNTOF(perfCounters), intersectionBranchBased, nv, ne,off, ind);
	Benchmark_TriangleCounting("CCT", "Branch-avoiding", perfCounters, COUNTOF(perfCounters), intersectionBranchAvoiding, nv, ne,off, ind);

#if defined(ARMASM)

	Benchmark_TriangleCounting("CCT", "Branch-avoiding-conditional", perfCounters, COUNTOF(perfCounters), intersectionBranchAvoidingArmAsm, nv, ne,off, ind);
#endif 


  }


#endif

  free(off);
  free(ind);
  return 0;
}



