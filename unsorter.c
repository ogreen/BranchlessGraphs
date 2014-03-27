//#include "timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <inttypes.h>
#include <assert.h>

static uint32_t nv, ne, naction;
static uint32_t * restrict off;
//static uint32_t * restrict from;
static uint32_t * restrict ind;
//static uint32_t * restrict weight;
//static uint32_t * restrict action;

static uint32_t * restrict indUnsorted;

static uint32_t * restrict permutation;

#define LINE_SIZE 10000


int main (const int argc, char *argv[]) {
		
	if (argc != 3) {
		fprintf(stderr, "Usage: a <graph-name> <unsorted-graph-name>\n");
		exit(EXIT_FAILURE);
	}
	char line[LINE_SIZE];

	FILE * fp = fopen(argv[1], "r");

	fgets(line, LINE_SIZE, fp);
	sscanf(line, "%d %d", &nv, &ne);
	nv += 1;
	off = memalign(64, (nv+2) * sizeof(uint32_t));
	ind = memalign(64, (ne*2) * sizeof(uint32_t));
	
	indUnsorted = memalign(64, (ne*2) * sizeof(uint32_t));	
	permutation = memalign(64, (nv+2) * sizeof(uint32_t));
	
	
	off[0]=0;
	off[1]=0;
	uint32_t counter=0;
	for (uint32_t u = 1; fgets(line, LINE_SIZE, fp); u++) {

		uint64_t neigh=0;
		uint64_t v = 0;
		char * ptr = line;
		int read = 0;

		while (sscanf(ptr, "%" SCNu64 "%n", &v, &read) > 0) {
			ptr += read;
			neigh++;
			ind[counter++]=v;
		}
		off[u+1]=off[u]+neigh;
	}

	fclose(fp);

//	printf("done reading the graph\n");

	srand(0);
	
	for(uint32_t u=1; u<nv; u++)
	{
//		printf("%d, ", u);fflush(stdout);
		uint32_t neigh=off[u+1]-off[u];
		for(uint32_t p=0; p<neigh; p++)
		{
			permutation[p]=1000000000;
		}

		for(uint32_t p=0; p<neigh; p++)
		{
			uint32_t r=rand()%neigh;
			while(permutation[r]!=1000000000)
			{
//				printf("%d, ", r);fflush(stdout);
				r=rand()%neigh;
			}
			permutation[r]=ind[off[u]+p];
		}

		for(uint32_t p=0; p<neigh; p++)
		{
			indUnsorted[off[u]+p]=permutation[p];
		}	
	}	

	
//	printf("done converting the graph\n");
	
	FILE * fp2 = fopen(argv[2], "w");

//	fprintf("oded\n");
	fprintf(fp2,"%d %d\n", nv, ne);
	
	for(uint32_t u=1; u<nv; u++)
	{
		uint32_t neigh=off[u+1]-off[u];	
		for(uint32_t p=0; p<neigh; p++)
		{
			fprintf(fp2,"%d ", indUnsorted[off[u]+p]);
		}	
	
		fprintf(fp2, "\n");
	}

	
	fclose(fp2);
	
	free(permutation);
	free(indUnsorted);

	free(off);
	free(ind);

	return 1;
	
}





