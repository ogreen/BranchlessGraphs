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


#if defined(BENCHMARK_CCT)
void Benchmark_TriangleCounting(const char* algorithm_name, const char* implementation_name, const struct PerformanceCounter performanceCounters[], size_t performanceCounterCount, CCT_Function cct_function, uint32_t numVertices, uint32_t numEdges,uint32_t* off, uint32_t* ind) {
  struct perf_event_attr perf_counter;


  uint32_t* triangles = (uint32_t*)memalign(64, numEdges * sizeof(uint32_t));
  uint32_t* intersectOps = (uint32_t*)memalign(64, numEdges * sizeof(uint32_t));        
  //        uint32_t* level = (uint32_t*)memalign(64, numVertices * sizeof(uint32_t));
  uint64_t* perf_events = (uint64_t*)malloc(numEdges * sizeof(uint64_t)*(performanceCounterCount));

  //printf("%d\n", performanceCounterCount);
  //return;

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
	for (size_t i = 0; i < numEdges; i++) {
	  triangles[i] = 0;
	  intersectOps[i] = 0;
	}

	int edgeCounter=0;           
	for (int src = 0; src < numVertices; src++){
	  int srcLen=off[src+1]-off[src];
	  for(int iter=off[src]; iter<off[src+1]; iter++){
		int dest=ind[iter];
		int destLen=off[dest+1]-off[dest];	

		struct timespec startTime;
		if (performanceCounters[performanceCounterIndex].type == PERF_TYPE_TIME) {
		  assert(clock_gettime(CLOCK_MONOTONIC, &startTime) == 0);
		} else {
		  assert(ioctl(perf_counter_fd, PERF_EVENT_IOC_RESET, 0) == 0);
		  assert(ioctl(perf_counter_fd, PERF_EVENT_IOC_ENABLE, 0) == 0);
		}

		triangles[edgeCounter]=cct_function(srcLen, ind+off[src], destLen, ind+off[dest]);
		intersectOps[edgeCounter]=srcLen+destLen;


		if (performanceCounters[performanceCounterIndex].type == PERF_TYPE_TIME) {
		  struct timespec endTime;
		  assert(clock_gettime(CLOCK_MONOTONIC, &endTime) == 0);
		  //                      printf("%d, ",numEdges * performanceCounterIndex + (edgeCounter)); fflush(stdout);
		  perf_events[numEdges * performanceCounterIndex + (edgeCounter)] =
			(1000000000ll * endTime.tv_sec + endTime.tv_nsec) - 
			(1000000000ll * startTime.tv_sec + startTime.tv_nsec);
		} else {
		  assert(ioctl(perf_counter_fd, PERF_EVENT_IOC_DISABLE, 0) == 0);
		  assert(read(perf_counter_fd, &perf_events[numEdges * performanceCounterIndex + (edgeCounter)], sizeof(uint64_t)) == sizeof(uint64_t));
		}
		edgeCounter++;

	  }
	}

	close(perf_counter_fd);
  }
  if(0){
	int edgeCounter=0;           
	for (int src = 0; src < numVertices; src++){
	  for(int iter=off[src]; iter<off[src+1]; iter++){
		printf("%s\t%s\t%"PRIu32"\t%"PRIu32, algorithm_name, implementation_name,numVertices,numEdges);
		for (size_t performanceCounterIndex = 0; performanceCounterIndex < performanceCounterCount; performanceCounterIndex++) {
		  if (!performanceCounters[performanceCounterIndex].supported)
			continue;
		  printf("\t%"PRIu64, perf_events[numEdges * performanceCounterIndex + edgeCounter]);
		}
		printf("\t%"PRIu32"\t%"PRIu32"\n", triangles[edgeCounter], intersectOps[edgeCounter]);
		//printf("\n");
		edgeCounter++;
	  }
	}
  }
  else
  {

	int edgeCounter=0;
	printf("%s\t%s\t%"PRIu32"\t%"PRIu32, algorithm_name, implementation_name,numVertices,numEdges);
	for (size_t performanceCounterIndex = 0; performanceCounterIndex < performanceCounterCount; performanceCounterIndex++) {
	  if (!performanceCounters[performanceCounterIndex].supported)
		continue;
	  edgeCounter=0;
	  int64_t val=0;
	  for (int src = 0; src < numVertices; src++){
		for(int iter=off[src]; iter<off[src+1]; iter++){
		  val+= perf_events[numEdges * performanceCounterIndex + edgeCounter];
		}
		edgeCounter++;
	  }
	  printf("\t%lld", val);
	}
	edgeCounter=0;
	int64_t tri=0,inter=0;
	for (int src = 0; src < numVertices; src++){
	  for(int iter=off[src]; iter<off[src+1]; iter++){
		tri+= triangles[edgeCounter];
		inter+=intersectOps[edgeCounter];
		edgeCounter++;
	  }
	}   
	printf("\t%lld", tri);
	printf("\t%lld", inter);
	printf("\n");

  }
  free(triangles);
  free(intersectOps);
  free(perf_events);

}

#endif



#include "cct.h"
#include "timer.h"

typedef enum{
	CCT_TT_BB,
	CCT_TT_BA,
	CCT_TT_BAC,	
	CCT_TT_MEM_ONLY,
	CCT_TT_INC,
	CCT_TT_ADD_1M,
	CCT_TT_ADD_VAR,
	CCT_TT_ADD_COND_EQ,
	CCT_TT_ADD_COND_GEQ,
	CCT_TT_ADD_COND_3,
	CCT_TT_ADD_BRANCH,
	CCT_TT_LAST,
} eCCTimers;

typedef struct{
	double cctTimers[CCT_TT_LAST];
	int32_t numberIntersections;
	int32_t numberBAWins;
	double ratioBAWins;		// BA over BB
	double ratioBACWins;	// BC over BB
	int32_t nv,ne;
} stats;


int32_t benchMarkMemoryAccess(int32_t* inPattern, int32_t sizeInPattern , int32_t sizeOut, stats* cctStats, int BranchVal);

void prettyPrint(stats printStats,char* graphName){

	printf("%25s, ", graphName);
	printf("%8s, ", "C");
	printf("%8d, ",printStats.nv);
	printf("%8d, ",printStats.ne);
	printf("%8d, ",printStats.numberIntersections);
	printf("%8d, ",printStats.numberBAWins);
	printf("%.5lf, ", printStats.cctTimers[CCT_TT_BB]);
	printf("%.5lf, ", printStats.cctTimers[CCT_TT_BA]);
	printf("%.5lf, ", printStats.cctTimers[CCT_TT_BAC]);
	printf("%.5lf, ",printStats.ratioBAWins);
	printf("%.5lf, ",printStats.ratioBACWins);

	// if (printStats.cctTimers[CCT_TT_BA]==0){
		// printf("Normalizing time unsucessful due to short baseTime - Graph probably too small\n");
		// return;
	// }

	// double memTime=printStats.cctTimers[CCT_TT_MEM_ONLY];
	// double baseTime=printStats.cctTimers[CCT_TT_INC]-memTime;

	// for(eCCTimers norm=CCT_TT_INC; norm<CCT_TT_LAST; norm++)
	// {
		// printf("%.5lf, ", ((printStats.cctTimers[norm]-memTime)/baseTime));
	// }

	printf("\n");
}

void prettyPrintSynthetic(stats printStats,char* benchmark,int length){

	printf("%8s, ", benchmark);
	printf("%8s, ", "C");
   

	double memTime=printStats.cctTimers[CCT_TT_MEM_ONLY];
	double baseTime=printStats.cctTimers[CCT_TT_INC]-memTime;

 //   printf("** %lf, %lf **,", memTime,baseTime);

	for(eCCTimers norm=CCT_TT_INC; norm<CCT_TT_LAST; norm++)
	{
		printf("%.5lf, ", ((printStats.cctTimers[norm]-memTime)/baseTime));
		//printf("%.5lf, ", ((printStats.cctTimers[norm]-memTime)));
		//printf("%.5lf, ", ((printStats.cctTimers[norm])));

	}
	printf("%10d, ",length);

	for(eCCTimers norm=CCT_TT_INC; norm<CCT_TT_LAST; norm++)
	{
		printf("%.5lf, ", ((printStats.cctTimers[norm])));
		//printf("%.5lf, ", ((printStats.cctTimers[norm]-memTime)));
		//printf("%.5lf, ", ((printStats.cctTimers[norm])));

	}


	printf("\n");
}

#if defined(X86)

int32_t intersectionBranchBased ( const int32_t alen, const int32_t * a,
						  const int32_t blen, const int32_t * b)
{
  int32_t ka = 0, kb = 0;
  int32_t out = 0;

  
  if (!alen || !blen || a[alen-1] < b[0] || b[blen-1] < a[0])
    return 0;

	const int32_t *aptr=a, *aend=a+alen;
	const int32_t *bptr=b, *bend=b+blen;

	while(aptr< aend && bptr<bend){
		if(*aptr==*bptr){
			aptr++, bptr++, out++;
		}
		else if(*aptr<*bptr){
			aptr++;
		}
		else {
			bptr++;
		}
  }  
  
	return out;
}
/*
int32_t intersectionBranchAvoiding ( const int32_t alen, const int32_t * a,  const int32_t blen, const  int32_t * b)
{
	int32_t ka = 0, kb = 0;
	int32_t out = 0;

	if (!alen || !blen || a[alen-1] < b[0] || b[blen-1] < a[0])
		return 0;
	int comp;

	const int32_t *aptr=a, *aend=a+alen;
	const int32_t *bptr=b, *bend=b+blen;

	while(aptr< aend && bptr<bend){
		comp   = (*aptr-*bptr);
//		aptr+= (comp<=0)?1:0;
//		bptr+= (comp>=0)?1:0;
//		out+= (comp==0)?1:0;		
   
		aptr+= (comp<=0);
		bptr+= (comp>=0);
		out+= (comp==0);		
   
	}

	
	return out;	
}
*/
int32_t intersectionBranchAvoiding ( const int32_t alen, const int32_t * a,  const int32_t blen, const  int32_t * b)
{
	int32_t ka = 0, kb = 0;
	int32_t out = 0;

	if (!alen || !blen || a[alen-1] < b[0] || b[blen-1] < a[0])
		return 0;
	uint32_t comp,comp2;

	const int32_t *aptr=a, *aend=a+alen;
	const int32_t *bptr=b, *bend=b+blen;
    int32_t asmaller,bsmaller;
	while(aptr< aend && bptr<bend){
		comp   = (*aptr-*bptr);
		comp2  = (*bptr-*aptr);
		asmaller= comp2>>31;
		bsmaller= comp>>31;
		aptr+=1-asmaller;
		bptr+=1-bsmaller;
		out+=1-asmaller-bsmaller;
//		printf("%d", asmaller);
//		aptr+= (comp<=0)?1:0;
//		bptr+= (comp>=0)?1:0;
//		out+= (comp==0)?1:0;		
   
//		aptr+= (comp<=0);
//		bptr+= (comp>=0);
//		out+= (comp==0);		
   
	}

	
	return out;	
}
 

#endif



#if defined(ARMASM)

int32_t intersectionBranchBased ( const int32_t alen, const int32_t * a,
						  const int32_t blen, const int32_t * b)
{
  int32_t ka = 0, kb = 0;
  int32_t out = 0;

  
  if (!alen || !blen || a[alen-1] < b[0] || b[blen-1] < a[0])
    return 0;

	const int32_t *aptr=a, *aend=a+alen;
	const int32_t *bptr=b, *bend=b+blen;

	while(aptr< aend && bptr<bend){
		if(*aptr==*bptr){
			aptr++, bptr++, out++;
		}
		else if(*aptr<*bptr){
			aptr++;
		}
		else {
			bptr++;
		}
  }  
  
	return out;
}

int32_t intersectionBranchAvoiding ( const int32_t alen, const int32_t * a,  const int32_t blen, const  int32_t * b)
{
  	int32_t ka = 0, kb = 0;
	int32_t out = 0;

	if (!alen || !blen || a[alen-1] < b[0] || b[blen-1] < a[0])
		return 0;
	int comp;

	while (ka < alen && kb < blen){
		comp   = (a[ka]-b[kb]);
//		ka+= (comp<=0)?1:0;
//		kb+= (comp>=0)?1:0;
//		out+= (comp==0)?1:0;		

		ka+= (comp<=0);
		kb+= (comp>=0);
		out+= (comp==0);		

	}

	return out;	  

}


int32_t intersectionBranchAvoidingArmAsm (const int32_t alen, const int32_t * a,
						  const int32_t blen, const int32_t * b)   {
  int32_t ka = 0, kb = 0;
  int32_t out = 0;
  if (!alen || !blen || a[alen-1] < b[0] || b[blen-1] < a[0])
		return 0;
	while (1){
		if(ka>=alen || kb>=blen){
			break;				
		}
		//comp   = (ind[apos+ka]-ind[bpos+kb]);
		
		int32_t vala=a[ka];
		int32_t valb=b[kb];
//		int32_t vala=ind[apos+ka];
//		int32_t valb=ind[bpos+kb];
		
		__asm__ __volatile__ (
			"CMP %[vala], %[valb];"
			"ADDEQ %[out], %[out], #1;"
			"ADDLS %[ka], %[ka], #1;"
			"ADDHS %[kb], %[kb], #1;"
			: [ka] "+r" (ka), [kb] "+r" (kb), [out] "+r" (out)
			: [vala] "r" (vala), [valb] "r" (valb)
		);

	}
	return out;	
	
}

#endif
 
int32_t intersectionCountMemOps ( const int32_t alen, const int32_t * a,
						  const int32_t blen, const int32_t * b)
{
  int32_t ka = 0, kb = 0;
  int32_t out = 0;
  
  int countMemOps=0;
  
  if (!alen || !blen || a[alen-1] < b[0] || b[blen-1] < a[0])
    return 0;

	while (ka < alen && kb < blen){

	if(a[ka]==b[kb]){
		countMemOps+=2;
		ka++,kb++, out++;
	}
	else if(a[ka]<b[kb]){
		countMemOps++;
		ka++;	
	}
	else {
		countMemOps++;
		kb++;	
	}
  }
	
	return countMemOps;
}
 
int32_t intersectionLogMemOps ( const int32_t alen, const int32_t * a,
						  const int32_t blen, const int32_t * b,
						 int32_t* memLog, int32_t apos, int32_t bpos,int32_t globalCounter)
{
  int32_t ka = 0, kb = 0;
  int32_t out = 0;
  
  int countMemOps=globalCounter;
  
  if (!alen || !blen || a[alen-1] < b[0] || b[blen-1] < a[0])
    return countMemOps;

  while (1) {
    if (ka >= alen || kb >= blen) break;

	if(a[ka]==b[kb]){
		memLog[countMemOps]=apos+ka;
		memLog[countMemOps+1]=apos+ka;
		countMemOps+=2;
		ka++,kb++, out++;
	
	}
	else if(a[ka]<b[kb]){
		memLog[countMemOps]=apos+ka;
		countMemOps++;
		ka++;	
	}
	else {
		memLog[countMemOps]=bpos+kb;
		countMemOps++;
		kb++;	
	}
  }
	
	return countMemOps;
}
 

void benchMarkCCT(const int32_t nv, const int32_t ne, const int32_t * off,   const int32_t * ind, int32_t * triNE,   int32_t* allTriangles,char* graphName, int32_t benchMarkSyn, int32_t synSize)
{
	int32_t edge=0;
	int32_t sum=0;

	double totalBB, totalBA,iterBB, iterBA, whenFaster,cc=0;
	int32_t triBB,triBA, triBAC,verTriangle=0, openTotal,temp;

	triBA=triBB=triBAC=0;
	whenFaster=totalBB=totalBA=iterBB=iterBA=0.0;
	int32_t countFaster=0;
	double totalBAC=0.0,iterBAC=0.0;
	int32_t countFasterBAC=0;

	
    for (int src = 0; src < nv; src++){
		int srcLen=off[src+1]-off[src];
		for(int iter=off[src]; iter<off[src+1]; iter++){
			tic();
			int dest=ind[iter];
			int destLen=off[dest+1]-off[dest];	
			triBA+=temp=intersectionBranchAvoiding(srcLen, ind+off[src], destLen, ind+off[dest]);
			iterBA=toc();
			totalBA+=iterBA;			

			tic();		
			dest=ind[iter];
			destLen=off[dest+1]-off[dest];
			triBB+= intersectionBranchBased (srcLen, ind+off[src], destLen, ind+off[dest]);			
			iterBB=toc();
			totalBB+=iterBB;

			if(iterBA<iterBB){
				countFaster++;
				whenFaster+=(iterBB-iterBA);
			}

			
#if defined( ARMASM)
			tic();
			dest=ind[iter];
			destLen=off[dest+1]-off[dest];	
			triBAC+=intersectionBranchAvoidingArmAsm(srcLen, ind+off[src], destLen, ind+off[dest]);
			iterBAC=toc();
			totalBAC+=iterBAC;

			if(iterBAC<iterBB){
				countFasterBAC++;
			}			
			if(triBA!=triBAC)
				printf("#");
			
#endif	
			
			sum+=triNE[edge++];
			verTriangle+=temp;
			
			if(triBA!=triBB)
				printf("*");
		}

		if(srcLen>1){
			openTotal+=(srcLen*srcLen-1);
			cc+=(double)verTriangle/(double)(srcLen*srcLen-1);
		}
		else
			openTotal++;

		verTriangle=0;
	}
    cc/=nv;

    stats cctStats;
    cctStats.cctTimers[CCT_TT_BB]=totalBB;
    cctStats.cctTimers[CCT_TT_BA]=totalBA;
    cctStats.cctTimers[CCT_TT_BAC]=totalBAC;

    cctStats.nv=nv;
    cctStats.ne=ne;

    cctStats.numberIntersections=edge;
    cctStats.numberBAWins=countFaster;
    cctStats.ratioBAWins=(double)countFaster/(double)edge;

	cctStats.ratioBACWins=0;
#if defined( ARMASM)
    cctStats.ratioBACWins=(double)countFasterBAC/(double)edge;
#endif

    //	printf("CC: %lf, Fraction: %lf\n",cc, (double)triBA/(double)openTotal);

    if(benchMarkSyn==1){
		int32_t memOpsCounter=0;
		for (int src = 0; src < nv; src++){
			int srcLen=off[src+1]-off[src];
			for(int iter=off[src]; iter<off[src+1]; iter++){
				int dest=ind[iter];
				int destLen=off[dest+1]-off[dest];	
				memOpsCounter+=intersectionCountMemOps (srcLen, ind+off[src], destLen, ind+off[dest]);	
			}
		}
		int32_t* logMem = (int32_t*)malloc(sizeof(int32_t)*(memOpsCounter+1)); 
		
		int pos=0;
		for (int src = 0; src < nv; src++)
		{
			int srcLen=off[src+1]-off[src];
			for(int iter=off[src]; iter<off[src+1]; iter++)
			{
				int dest=ind[iter];
				int destLen=off[dest+1]-off[dest];	
				pos=intersectionLogMemOps(srcLen, ind+off[src], destLen, ind+off[dest], logMem, off[src], off[dest],pos);	
				//printf("%d, ", pos);
			}
		}
		
    	benchMarkMemoryAccess(logMem,synSize,ne,&cctStats, nv/4);
    	prettyPrintSynthetic(cctStats,"Mix",synSize);
		free(logMem);		
    }
    else{

		prettyPrint(cctStats,graphName);
		
    }

 

	*allTriangles=sum;
}

void benchMarkLinear(int32_t length){
	stats cctStats;

	int32_t* logMem = (int32_t*)malloc(sizeof(int32_t)*(length+1));


	for(int32_t i=0; i<length; i++)
		logMem[i]=i;

	benchMarkMemoryAccess(logMem,length,length,&cctStats, length/2);

	prettyPrintSynthetic(cctStats,"Linear",length);

	free(logMem);
}

void benchMarkRandom(int32_t length){
	stats cctStats;

	int32_t* logMem = (int32_t*)malloc(sizeof(int32_t)*(length+1));

	for(int32_t i=0; i<length; i++)
		logMem[i]=rand()%length;
	benchMarkMemoryAccess(logMem,length,length,&cctStats, length/2);
	prettyPrintSynthetic(cctStats,"Random",length);
	free(logMem);
}

void benchMarkAllSynthetic(const int32_t nv, const int32_t ne, const int32_t * off, const int32_t * ind,int32_t * triNE,int32_t length, char* graphName){
	  time_t t;
	  srand((unsigned) time(NULL));
	  int32_t allTrianglesCPU=0;
	  benchMarkLinear(length);
	  benchMarkCCT( nv, ne, off,ind, triNE, &allTrianglesCPU,graphName, 1, length);
	  benchMarkRandom(length);

}

void resetOutput(int32_t* out, int32_t len){
  for(int i=0;i<len; i++)
	out[i]=0;
}
int32_t sumOutput(int32_t* out, int32_t len){
  int32_t sum=0;
  for(int i=0;i<len; i++)
	sum+=out[i];
}
 

int32_t benchMarkMemoryAccess(int32_t* inPattern, int32_t sizeInPattern , int32_t sizeOut, stats* cctStats, int branchVal)
{
	int32_t* outArray = (int32_t*)malloc(sizeof(int32_t)*sizeOut);

//	printf("%d   %d\n",sizeInPattern,sizeOut);
//	for(int i=0; i< 30;i++)
//	{
//		printf("%d, ", inPattern[i]);
//	}
//	printf("\n");

	int sum=0;
     for(int m=0; m<sizeInPattern;m++)
    	outArray[inPattern[m]]=0;
    cctStats->cctTimers[CCT_TT_MEM_ONLY]=toc();
     for(int m=sizeInPattern; m>0;m--)
    	outArray[inPattern[m]]=0;
    cctStats->cctTimers[CCT_TT_MEM_ONLY]=toc();
     for(int m=0; m<sizeInPattern;m++)
    	outArray[inPattern[m]]=0;
    cctStats->cctTimers[CCT_TT_MEM_ONLY]=toc();

 
    resetOutput(outArray,sizeOut);
	tic();
    for(int m=0; m<sizeInPattern;m++)
    	outArray[inPattern[m]]=0;
    cctStats->cctTimers[CCT_TT_MEM_ONLY]=toc();

	sum+=sumOutput(outArray,sizeOut);
	

    resetOutput(outArray,sizeOut);
    tic();
    for(int m=0; m<sizeInPattern;m++)
    	outArray[inPattern[m]]+=1;
    cctStats->cctTimers[CCT_TT_INC]=toc();
	sum+=sumOutput(outArray,sizeOut);

    resetOutput(outArray,sizeOut);
    tic();
    for(int m=0; m<sizeInPattern;m++)
    	outArray[inPattern[m]]+=1000000;
    cctStats->cctTimers[CCT_TT_ADD_1M]=toc();

	sum+=sumOutput(outArray,sizeOut);
    resetOutput(outArray,sizeOut);
    tic();
    for(int m=0; m<sizeInPattern;m++)
    	outArray[inPattern[m]]+=sizeInPattern;
    cctStats->cctTimers[CCT_TT_ADD_VAR]=toc();


	sum+=sumOutput(outArray,sizeOut);
    resetOutput(outArray,sizeOut);
    tic();
    for(int m=0; m<sizeInPattern;m++)
    	outArray[inPattern[m]]+=(inPattern[m]<=branchVal   );
    cctStats->cctTimers[CCT_TT_ADD_COND_GEQ]=toc();
	
	
	sum+=sumOutput(outArray,sizeOut);
    resetOutput(outArray,sizeOut);
    tic();
    for(int m=0; m<sizeInPattern;m++)
    	outArray[inPattern[m]]+=(inPattern[m]==branchVal);
    cctStats->cctTimers[CCT_TT_ADD_COND_EQ]=toc();

	
	sum+=sumOutput(outArray,sizeOut);
    resetOutput(outArray,sizeOut);
    tic();
    for(int m=0; m<sizeInPattern;m++) {
    	outArray[inPattern[m]]+=(inPattern[m]==branchVal)+(inPattern[m]>=branchVal)+2*(inPattern[m]<branchVal);
	}
    cctStats->cctTimers[CCT_TT_ADD_COND_3]=toc();

	sum+=sumOutput(outArray,sizeOut);

    resetOutput(outArray,sizeOut);
    tic();
    for(int m=0; m<sizeInPattern;m++) {
		if(inPattern[m]>=branchVal)
			outArray[inPattern[m]]+=branchVal;
		else 
			outArray[inPattern[m]]+=1;
	}
    cctStats->cctTimers[CCT_TT_ADD_BRANCH]=toc();

	sum+=sumOutput(outArray,sizeOut);

    free(outArray);
	return sum;
}
