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
	int32_t numberIntersections;
	int32_t numberBAWins;
	double ratioBAWins;		// BA over BA
	int32_t nv,ne;
} stats;


void benchMarkMemoryAccess(int* inPattern, int sizeInPattern , int sizeOut, stats* cctStats);

void prettyPrint(stats printStats,char* graphName){

	printf("%25s, ", graphName);
	printf("%8s, ", "C");
	printf("%8d, ",printStats.nv);
	printf("%8d, ",printStats.ne);
	printf("%8d, ",printStats.numberIntersections);
	printf("%8d, ",printStats.numberBAWins);
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

void prettyPrintSynthetic(stats printStats,char* benchmark,int length){

	printf("%8s, ", benchmark);
	printf("%8s, ", "C");
	printf("%8d, ",length);

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


int32_t intersectionBranchBased ( const int32_t alen, const int32_t * a,
						  const int32_t blen, const int32_t * b)
{
  int32_t ka = 0, kb = 0;
  int32_t out = 0;

  
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

int32_t intersectionBranchAvoiding ( const int32_t alen, const int32_t apos,
						  const int32_t blen, const int32_t bpos, const int32_t* ind)
{
  int32_t ka = 0, kb = 0;
  int32_t out = 0;
int comp;
	if(alen==0 || blen==0 || ind[apos+alen-1]<ind[bpos] || ind[bpos + blen-1]<ind[apos] )
		return 0;

	while (1){
		if(ka>=alen || kb>=blen){
			break;				
		}
		comp   = (ind[apos+ka]-ind[bpos+kb]);
		ka+= (comp<=0)?1:0;
		kb+= (comp>=0)?1:0;
		out+= (comp==0)?1:0;							
	}
	return out;	
	
}


int32_t intersectionCountMemOps ( const int32_t alen, const int32_t * a,
						  const int32_t blen, const int32_t * b)
{
  int32_t ka = 0, kb = 0;
  int32_t out = 0;
  
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
 

void triangleCountBranchBased(const int32_t nv, const int32_t ne, const int32_t * off,   const int32_t * ind, int32_t * triNE,   int32_t* allTriangles,char* graphName)
{
	int32_t edge=0;
	int32_t sum=0;

	double totalBB, totalBA,iterBB, iterBA, whenFaster,cc=0;
	int32_t triBB,triBA, verTriangle=0, openTotal,temp;

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
			triBA+=temp=intersectionBranchAvoiding(srcLen, off[src], destLen, off[dest],ind);
			//triBA+= intersectionBranchAvoiding (srcLen, ind+off[src], destLen, ind+off[dest]);
			iterBA=toc();
			totalBA+=iterBA;			

			tic();		
			dest=ind[iter];
			destLen=off[dest+1]-off[dest];
			triBB= intersectionBranchBased (srcLen, ind+off[src], destLen, ind+off[dest]);			

			iterBB=toc();
			totalBB+=iterBB;

			memOpsCounter+=intersectionCountMemOps   (srcLen, ind+off[src], destLen, ind+off[dest]);

			if(iterBA<iterBB){
				countFaster++;
				whenFaster+=(iterBB-iterBA);
			}
			sum+=triNE[edge++];
			verTriangle+=temp;
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

    cctStats.nv=nv;
    cctStats.ne=ne;

    cctStats.numberIntersections=edge;
    cctStats.numberBAWins=countFaster;
    cctStats.ratioBAWins=(double)countFaster/(double)edge;



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

    //	printf("CC: %lf, Fraction: %lf\n",cc, (double)triBA/(double)openTotal);
    int32_t* vertexAccess = (int32_t*)malloc(sizeof(int32_t)*off[nv]);

//    memset(vertexAccess,0, off[nv]*sizeof(int32_t));
//    memset(vertexAccess,0, off[nv]*sizeof(int32_t)); tic(); for(int m=0; m<memOpsCounter;m++) vertexAccess[logMem[m]]; cctStats.cctTimers[CCT_TT_MEM_ONLY]=toc();
//    memset(vertexAccess,0, off[nv]*sizeof(int32_t)); tic(); for(int m=0; m<memOpsCounter;m++) vertexAccess[logMem[m]]++; cctStats.cctTimers[CCT_TT_INC]=toc();
//    memset(vertexAccess,0, off[nv]*sizeof(int32_t)); tic(); for(int m=0; m<memOpsCounter;m++) vertexAccess[logMem[m]]+=1000000; cctStats.cctTimers[CCT_TT_ADD_1M]=toc();
//    memset(vertexAccess,0, off[nv]*sizeof(int32_t)); tic(); for(int m=0; m<memOpsCounter;m++) vertexAccess[logMem[m]]+=memOpsCounter; cctStats.cctTimers[CCT_TT_ADD_VAR]=toc();
//    memset(vertexAccess,0, off[nv]*sizeof(int32_t)); tic(); for(int m=0; m<memOpsCounter;m++) vertexAccess[logMem[m]]+=(vertexAccess[logMem[m]]==0); cctStats.cctTimers[CCT_TT_ADD_COND]=toc();
//         int32_t a=0,b=0,c=0;
//    memset(vertexAccess,0, off[nv]*sizeof(int32_t)); tic(); for(int m=0; m<memOpsCounter;m++) {a+=vertexAccess[logMem[m]]==0;b+=vertexAccess[logMem[m]]>=0;c+=vertexAccess[logMem[m]]<=0;} cctStats.cctTimers[CCT_TT_ADD_COND_3]=toc();
//    vertexAccess[0]=a+b+c;
    benchMarkMemoryAccess(logMem,memOpsCounter,ne,&cctStats);


    free(vertexAccess);

	prettyPrint(cctStats,graphName);


    free(logMem);

	*allTriangles=sum;
}



void benchMarkMemoryAccess(int32_t* inPattern, int32_t sizeInPattern , int32_t sizeOut, stats* cctStats)
{
	int32_t* outArray = (int32_t*)malloc(sizeof(int32_t)*sizeOut);

    memset(outArray,0, sizeOut*sizeof(int32_t));

    memset(outArray,0, sizeOut*sizeof(int32_t));
    tic();
    for(int m=0; m<sizeInPattern;m++) outArray[inPattern[m]];
    cctStats->cctTimers[CCT_TT_MEM_ONLY]=toc();

    memset(outArray,0, sizeOut*sizeof(int32_t));
    tic();
    for(int m=0; m<sizeInPattern;m++)
    	outArray[inPattern[m]]++;
    cctStats->cctTimers[CCT_TT_INC]=toc();

    memset(outArray,0, sizeOut*sizeof(int32_t));
    tic();
    for(int m=0; m<sizeInPattern;m++)
    	outArray[inPattern[m]]+=1000000;
    cctStats->cctTimers[CCT_TT_ADD_1M]=toc();

    memset(outArray,0, sizeOut*sizeof(int32_t));
    tic();
    for(int m=0; m<sizeInPattern;m++)
    	outArray[inPattern[m]]+=sizeInPattern;
    cctStats->cctTimers[CCT_TT_ADD_VAR]=toc();

    memset(outArray,0, sizeOut*sizeof(int32_t));
    tic();
    for(int m=0; m<sizeInPattern;m++)
    	outArray[inPattern[m]]+=(outArray[inPattern[m]]==0);
    cctStats->cctTimers[CCT_TT_ADD_COND]=toc();

    int32_t a=0,b=0,c=0;
    memset(outArray,0, sizeOut*sizeof(int32_t));
    tic();
    for(int m=0; m<sizeInPattern;m++) {
    	a+=outArray[inPattern[m]]==0;
    	b+=outArray[inPattern[m]]>=0;
    	c+=outArray[inPattern[m]]<=0;}
    outArray[0]=a+b+c;
    cctStats->cctTimers[CCT_TT_ADD_COND_3]=toc();



    free(outArray);


}
