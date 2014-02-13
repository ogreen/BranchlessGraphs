#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#ifdef __x86_64__
	#include <x86intrin.h>
#endif
#include "timer.h"

uint32_t BFSSeqBULevelInformation(uint32_t* off, uint32_t* ind, uint32_t* bitmap, uint32_t* level, uint32_t currRoot, uint32_t nv,uint32_t currLevel,uint32_t nextLevel, uint32_t* edgesTraversed, uint32_t* verticesFound) {
	
	uint32_t changed=0;

	uint32_t v;
	
//	uint32_t qEnd=1;
	verticesFound[currLevel]=0;
	edgesTraversed[currLevel]=0;

		for(v=0; v<nv; v++)
		{
			
			if(!bitmap[v])
			{
				uint32_t startEdge = off[v];
				uint32_t stopEdge = off[v+1];
				uint32_t j;
				for (j = startEdge; startEdge < stopEdge; startEdge++) {
					uint32_t k = ind[startEdge];
		
					// If this is a neighbor and has not been found
					if (level[k] == currLevel) {
							level[v] = nextLevel;
							bitmap[v]=1;
							changed=1;
//							qEnd++;
							//printf("%d,",qEnd);
							verticesFound[currLevel]++;
							edgesTraversed[currLevel]+=j;
							break;
					}
				}
			}
		}
		return changed;
//	printf("\nQE %d %d\n",qEnd,currLevel-1);
}

uint32_t BFSSeqBU(uint32_t* off, uint32_t* ind, uint32_t* bitmap, uint32_t* level, uint32_t currRoot, uint32_t nv,uint32_t currLevel,uint32_t nextLevel) {
	
	uint32_t changed=0;

	uint32_t v;
	
//	uint32_t qEnd=1;

		for(v=0; v<nv; v++)
		{
			
			if(!bitmap[v])
			{
				uint32_t startEdge = off[v];
				uint32_t stopEdge = off[v+1];
				uint32_t j;
				for (j = startEdge; startEdge < stopEdge; startEdge++) {
					uint32_t k = ind[startEdge];
		
					// If this is a neighbor and has not been found
					if (level[k] == currLevel) {
							level[v] = nextLevel;
							bitmap[v]=1;
							changed=1;
//							qEnd++;
							//printf("%d,",qEnd);
							break;
					}
				}
			}
		}
		return changed;
//	printf("\nQE %d %d\n",qEnd,currLevel-1);
}


