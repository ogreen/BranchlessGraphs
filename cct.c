#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cct.h"
#include "timer.h"


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

//int32_t counter=0;
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
		ka++,kb++, out++;
	}
	else if(a[ka]<b[kb]){
		ka++;	
	}
	else {
		kb++;	
	}
    countMemOps++;
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
		ka++,kb++, out++;
	
	}
	else if(a[ka]<b[kb]){
		memLog[countMemOps]=apos+ka;
		ka++;	
	}
	else {
		memLog[countMemOps]=bpos+kb;
		kb++;	
	}
    countMemOps++;
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
			int dest=ind[iter];
			int destLen=off[dest+1]-off[dest];	


			tic();
			triNE[edge]= intersectionBranchAvoiding (src, srcLen, ind+off[src], dest, destLen, ind+off[dest]);
			iterBA=toc();
			totalBA+=iterBA;			
			triBA+=triNE[edge];

			tic();		
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

	printf("Branched-based       : %lf\n", totalBB);
	printf("Branched-avoiding    : %lf\n", totalBA);
	printf("Faster intersections : %lf\n", (double)countFaster/(double)edge);
	printf("Results are equal    : %d\n", triBA==triBB);
	printf("Results are equal    : %ld %ld\n", triBB,triBA);
	printf("When faster          : %lf\n", whenFaster);

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
	memset(vertexAccess,0, off[nv]*sizeof(int32_t)); tic();	for(int m=0; m<memOpsCounter;m++) vertexAccess[logMem[m]]; timeNOP=toc();
	memset(vertexAccess,0, off[nv]*sizeof(int32_t)); tic();	for(int m=0; m<memOpsCounter;m++) vertexAccess[logMem[m]]++; timeInc=toc();
	memset(vertexAccess,0, off[nv]*sizeof(int32_t)); tic();	for(int m=0; m<memOpsCounter;m++) vertexAccess[logMem[m]]+=1; timeAdd1=toc();
	memset(vertexAccess,0, off[nv]*sizeof(int32_t)); tic();	for(int m=0; m<memOpsCounter;m++) vertexAccess[logMem[m]]+=255; timeAdd255=toc();
	memset(vertexAccess,0, off[nv]*sizeof(int32_t)); tic();	for(int m=0; m<memOpsCounter;m++) vertexAccess[logMem[m]]+=100000; timeAdd1M=toc();
	memset(vertexAccess,0, off[nv]*sizeof(int32_t)); tic();	for(int m=0; m<memOpsCounter;m++) vertexAccess[logMem[m]]+=memOpsCounter; timeAddVar=toc();
	memset(vertexAccess,0, off[nv]*sizeof(int32_t)); tic();	for(int m=0; m<memOpsCounter;m++) vertexAccess[logMem[m]]+=(vertexAccess[logMem[m]]==0); timeAddCondEq0=toc();
	memset(vertexAccess,0, off[nv]*sizeof(int32_t)); tic();	for(int m=0; m<memOpsCounter;m++) vertexAccess[logMem[m]]+=(vertexAccess[logMem[m]]!=0); timeAddCondNEq0=toc();
	memset(vertexAccess,0, off[nv]*sizeof(int32_t)); tic();	for(int m=0; m<memOpsCounter;m++) vertexAccess[logMem[m]]+=(vertexAccess[logMem[m]]==0+vertexAccess[logMem[m]]>=0+vertexAccess[logMem[m]]<=0); timeAdd3Cond=toc();
   // memset(vertexAccess,0, off[nv]*sizeof(int32_t)); tic();	for(int m=0; m<memOpsCounter;m++) vertexAccess[logMem[m]]+=(vertexAccess[logMem[m]]==0+vertexAccess[logMem[m]]>=0+vertexAccess[logMem[m]]<=0); timeAdd3Cond=toc();
    int32_t a=0,b=0,c=0;
	memset(vertexAccess,0, off[nv]*sizeof(int32_t)); tic();	for(int m=0; m<memOpsCounter;m++) {a+=vertexAccess[logMem[m]]==0;b+=vertexAccess[logMem[m]]>=0;c+=vertexAccess[logMem[m]]<=0;} timeAdd3Cond=toc();
	vertexAccess[0]=a+b+c;	
	printf("\n\n");
	printf("Memops               : %d\n", memOpsCounter);
	printf("NOP                  : %lf\n", timeNOP);
	printf("Inc                  : %lf\n", timeInc);
	printf("Add1                 : %lf\n", timeAdd1);
	printf("Add255               : %lf\n", timeAdd255);
	printf("Add1M                : %lf\n", timeAdd1M);
	printf("AddVar               : %lf\n", timeAddVar);
	printf("AddCondEq0           : %lf\n", timeAddCondEq0);
	printf("AddCondNEq0          : %lf\n", timeAddCondNEq0);
	printf("Add3Cond             : %lf\n", timeAdd3Cond);

	
	for(int m=0; m<memOpsCounter;m++) {
	  
	}; 
	printf("\n");
    free(vertexAccess);

    free(logMem);

/*    for (int32_t i = 0; i < N; ++i)
    {
      ntri[i]= count_triangles (nv, off, ind, i,numIntersections,possibleTriangles,elementsIntersected);
    }
*/  
/*	*allTriangles=0;
    for (int32_t i = 0; i < N; ++i)
      *allTriangles+=ntri[i];
*/
//	printf("CPU SUM %d\n", sum);
	*allTriangles=sum;
}


