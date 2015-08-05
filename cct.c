#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "cct.h"
#include "timer.h"

typedef enum{
	CCT_TT_BB,
	CCT_TT_BA,
	CCT_TT_MEM_ONLY,
	CCT_TT_INC,
	CCT_TT_ADD_1M,
	CCT_TT_ADD_VAR,
	CCT_TT_ADD_COND,
	CCT_TT_ADD_COND_3,
	CCT_TT_LAST,
} eCCTimers;

typedef struct{
	double cctTimers[CCT_TT_LAST];
	int64_t numberIntersections;
	int64_t numberBAWins;
	double ratioBAWins;		// BA over BA
	int32_t nv,ne;
} stats;

void prettyPrint(stats printStats){

	printf("%8s, ", "C");
//	printf("%s", graphName)
	printf("%8d, ",printStats.nv);
	printf("%8d, ",printStats.ne);
	printf("%8ld, ",printStats.numberIntersections);
	printf("%8ld, ",printStats.numberBAWins);
	printf("%.5lf, ", printStats.cctTimers[CCT_TT_BB]);
	printf("%.5lf, ", printStats.cctTimers[CCT_TT_BA]);
	printf("%.5lf, ",printStats.ratioBAWins);

	if (printStats.cctTimers[CCT_TT_BA]==0){
		printf("Normalizing time unsucessful due to short baseTime - Graph probably too small\n");
		return;
	}

	double memTime=printStats.cctTimers[CCT_TT_MEM_ONLY];
	double baseTime=printStats.cctTimers[CCT_TT_INC]-memTime;

	for(eCCTimers norm=CCT_TT_INC; norm<CCT_TT_LAST; norm++)
	{
		printf("%.5lf, ", ((printStats.cctTimers[norm]-memTime)/baseTime));

	}

	printf("\n");
}

int64_t intersectionBranchBased (const int32_t ai, const int32_t alen, const int32_t * a,
						 const int32_t bi, const int32_t blen, const int32_t * b)
{
  int32_t ka = 0, kb = 0;
  int64_t out = 0;

  
  if (!alen || !blen || a[alen-1] < b[0] || b[blen-1] < a[0])
    return 0;

  while (1) {
    if (ka >= alen || kb >= blen) break;

	if(a[ka]==b[kb]){
		ka++,kb++, out++;
	}
	else if(a[ka]<b[kb]){
		ka++;	
	}
	else {
		kb++;	
	}
  }

	return out;
}

int64_t intersectionBranchAvoiding (const int32_t ai, const int32_t alen, const int32_t * a,
						 const int32_t bi, const int32_t blen, const int32_t * b)
{
  int32_t ka = 0, kb = 0;
  int64_t out = 0;

  
  if (!alen || !blen || a[alen-1] < b[0] || b[blen-1] < a[0])
    return 0;

  while (1) {
    if (ka >= alen || kb >= blen) break;

	int32_t comp   = (a[ka]-b[kb]);


	out+= comp==0;
	ka+= (comp<=0);
	kb+= (comp>=0);


  }

//	counter=10;
	
	return out;
}


int64_t intersectionCountMemOps (const int32_t ai, const int32_t alen, const int32_t * a,
						 const int32_t bi, const int32_t blen, const int32_t * b)
{
  int32_t ka = 0, kb = 0;
  int64_t out = 0;
  
  int countMemOps=0;
  
  if (!alen || !blen || a[alen-1] < b[0] || b[blen-1] < a[0])
    return 0;

  while (1) {
    if (ka >= alen || kb >= blen) break;

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
 
int32_t intersectionLogMemOps (const int32_t ai, const int32_t alen, const int32_t * a,
						 const int32_t bi, const int32_t blen, const int32_t * b,
						 int32_t* memLog, int32_t apos, int32_t bpos,int32_t globalCounter)
{
  int32_t ka = 0, kb = 0;
  int64_t out = 0;
  
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
 

void triangleCountBranchBased(const int32_t nv, const int32_t * off,
    const int32_t * ind, int64_t * triNE,
    int64_t* allTriangles)
{
	int32_t edge=0;
	int64_t sum=0;

	double totalBB, totalBA,iterBB, iterBA, whenFaster;
	int64_t triBB,triBA;
	
	triBA=triBB=0;
	whenFaster=totalBB=totalBA=iterBB=iterBA=0.0;
	int32_t countFaster=0;

	int32_t memOpsCounter=0;
    for (int src = 0; src < nv; src++)
    {
		int srcLen=off[src+1]-off[src];

		for(int iter=off[src]; iter<off[src+1]; iter++)
		{
			tic();
			int dest=ind[iter];
			int destLen=off[dest+1]-off[dest];	
			triNE[edge]= intersectionBranchAvoiding (src, srcLen, ind+off[src], dest, destLen, ind+off[dest]);
			iterBA=toc();
			totalBA+=iterBA;			
			triBA+=triNE[edge];

			tic();		
			dest=ind[iter];
			destLen=off[dest+1]-off[dest];
			triNE[edge]= intersectionBranchBased (src, srcLen, ind+off[src], dest, destLen, ind+off[dest]);			

			iterBB=toc();
			totalBB+=iterBB;
			triBB+=triNE[edge];

			memOpsCounter+=intersectionCountMemOps   (src, srcLen, ind+off[src], dest, destLen, ind+off[dest]);	

			if(iterBA<iterBB){
				countFaster++;
				whenFaster+=(iterBB-iterBA);
			}
			sum+=triNE[edge++];
		}
	}

    stats cctStats;
    cctStats.cctTimers[CCT_TT_BB]=totalBB;
    cctStats.cctTimers[CCT_TT_BA]=totalBA;

    cctStats.nv=nv;
    cctStats.ne=off[nv];

    cctStats.numberIntersections=edge;
    cctStats.numberBAWins=countFaster;
    cctStats.ratioBAWins=(double)countFaster/(double)edge;



	int32_t* logMem = (int32_t*)malloc(sizeof(int32_t)*(memOpsCounter+1)); 
	int32_t* vertexAccess = (int32_t*)malloc(sizeof(int32_t)*off[nv]); 
	
	int pos=0;
    for (int src = 0; src < nv; src++)
    {
		int srcLen=off[src+1]-off[src];
		for(int iter=off[src]; iter<off[src+1]; iter++)
		{
			int dest=ind[iter];
			int destLen=off[dest+1]-off[dest];	
		    pos=intersectionLogMemOps(src, srcLen, ind+off[src], dest, destLen, ind+off[dest], logMem, off[src], off[dest],pos);	
			//printf("%d, ", pos);
		}
	}

	memset(vertexAccess,0, off[nv]*sizeof(int32_t));

	double timeNOP=0,timeInc=0, timeAdd1=0, timeAdd255=0, timeAdd1M=0, timeAddVar=0,timeAddCondEq0=0,timeAddCondNEq0=0,timeAdd3Cond=0;
	memset(vertexAccess,0, off[nv]*sizeof(int32_t)); tic();	for(int m=0; m<memOpsCounter;m++) vertexAccess[logMem[m]]; cctStats.cctTimers[CCT_TT_MEM_ONLY]=toc();
	memset(vertexAccess,0, off[nv]*sizeof(int32_t)); tic();	for(int m=0; m<memOpsCounter;m++) vertexAccess[logMem[m]]++; cctStats.cctTimers[CCT_TT_INC]=toc();
	memset(vertexAccess,0, off[nv]*sizeof(int32_t)); tic();	for(int m=0; m<memOpsCounter;m++) vertexAccess[logMem[m]]+=1000000; cctStats.cctTimers[CCT_TT_ADD_1M]=toc();
	memset(vertexAccess,0, off[nv]*sizeof(int32_t)); tic();	for(int m=0; m<memOpsCounter;m++) vertexAccess[logMem[m]]+=memOpsCounter; cctStats.cctTimers[CCT_TT_ADD_VAR]=toc();
	memset(vertexAccess,0, off[nv]*sizeof(int32_t)); tic();	for(int m=0; m<memOpsCounter;m++) vertexAccess[logMem[m]]+=(vertexAccess[logMem[m]]==0); cctStats.cctTimers[CCT_TT_ADD_COND]=toc();
    int32_t a=0,b=0,c=0;
	memset(vertexAccess,0, off[nv]*sizeof(int32_t)); tic();	for(int m=0; m<memOpsCounter;m++) {a+=vertexAccess[logMem[m]]==0;b+=vertexAccess[logMem[m]]>=0;c+=vertexAccess[logMem[m]]<=0;} cctStats.cctTimers[CCT_TT_ADD_COND_3]=toc();
	vertexAccess[0]=a+b+c;	

	prettyPrint(cctStats);

    free(vertexAccess);
    free(logMem);

	*allTriangles=sum;
}



