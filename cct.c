#include <stdio.h>

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
		ka++;kb++;out++;
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
	ka+= (comp<=0);
	kb+= (comp>=0);
	out+= (comp==0);
    // if (ka >= alen || kb >= blen) break;
	// int32_t comp2  = (a[ka]-b[kb]);

	// ka+= (comp2<=0);
	// kb+= (comp2>=0);
 
	// out+= (comp2==0);
	
  }

//	counter=10;
	
	return out;
}


void triangleCountBranchBased(const int32_t nv, const int32_t * off,
    const int32_t * ind, int64_t * triNE,
    int64_t* allTriangles)
{
	int32_t edge=0;
	int64_t sum=0;
	double totalBB, totalBA,iterBB, iterBA, whenFaster, ccGlobal;
	int64_t triBB,triBA, actualTriangles, possibleTriangles, globalPossibleTriangles;
	
	triBA=triBB=actualTriangles=possibleTriangles=globalPossibleTriangles=0;
	whenFaster=totalBB=totalBA=iterBB=iterBA=ccGlobal=0.0;
	int32_t countFaster=0;
	
    for (int src = 0; src < nv; src++)
    {
		int srcLen=off[src+1]-off[src];
		actualTriangles=0;
		possibleTriangles=srcLen*srcLen;
		globalPossibleTriangles+=possibleTriangles;
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
			triNE[edge]= intersectionBranchBased    (src, srcLen, ind+off[src], dest, destLen, ind+off[dest]);			
			iterBB=toc();
			totalBB+=iterBB;
			triBB+=triNE[edge];


			if(iterBA<iterBB){
				countFaster++;
				whenFaster+=(iterBB-iterBA);
			}
			actualTriangles+=triNE[edge];
			
			sum+=triNE[edge++];
		}
		if(possibleTriangles!=0)
			ccGlobal+= (double)actualTriangles/(double)possibleTriangles;
	}
	ccGlobal/=nv;

	printf("Branched-based       : %lf\n", totalBB);
	printf("Branched-avoiding    : %lf\n", totalBA);
	printf("Faster intersections : %.2lf \% \n", 100*(double)countFaster/(double)edge);
	printf("When faster          : %lf\n", whenFaster);
	printf("Results are equal    : %ld %ld\n", triBB,triBA);
	printf("Open triangles       : %ld \n", globalPossibleTriangles);
	printf("Ratio closed 	     : %.2lf \n",ccGlobal);

//	int64_t BM=10;
	int64_t closedTriangles=triBB;
	int64_t totalBBCycles = 3E9*totalBB;
	int64_t BM= 2*(totalBBCycles-2*(globalPossibleTriangles+closedTriangles))/ (globalPossibleTriangles+closedTriangles);
	printf("BM %ld\n", BM);
	int64_t modelBB=triBB*(BM + 4) + (globalPossibleTriangles-closedTriangles)*(2+0.5*BM);
	int64_t modelBA=6*globalPossibleTriangles;

	printf("Branch-based model   : %ld\n", modelBB);
	printf("Branch-avoiding model: %ld\n", modelBA);
	
	
	
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




 /*
int64_t intersectionBranchAvoiding (const int32_t ai, const int32_t alen, const int32_t * a,
						 const int32_t bi, const int32_t blen, const int32_t * b)
{
  int32_t ka = 0, kb = 0;
  int64_t out = 0;

  
  if (!alen || !blen || a[alen-1] < b[0] || b[blen-1] < a[0])
    return 0;

  while (1) {
    if (ka >= alen || kb >= blen) break;



	int32_t condEq   = ((a[ka]-b[kb])==0);
	int32_t condDiff = ((uint32_t)(a[ka]-b[kb])>>31);

	out+= condEq;
	ka+= (condEq || condDiff);
	kb+= (condEq || (1-condDiff));

	
  }

//	counter=10;
	
	return out;
}
*/ 

