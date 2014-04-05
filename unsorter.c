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


void readGraphDIMACS(char* filePath, int32_t** prmoff,  int32_t** prmind, int32_t* prmnv,int32_t* prmne){
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
		
	if (argc != 3) {
		fprintf(stderr, "Usage: a <graph-name> <unsorted-graph-name>\n");
		exit(EXIT_FAILURE);
	}

	readGraphDIMACS(argv[1], &off,  &ind, &nv,&ne);

	indUnsorted = memalign(64, (ne*2) * sizeof(uint32_t));	
	permutation = memalign(64, (nv+2) * sizeof(uint32_t));
	
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

	printf("done converting the graph\n");
	
	FILE * fp2 = fopen(argv[2], "w");

//	fprintf("oded\n");
	fprintf(fp2,"%d %d 0\n", nv-1, ne/2);
	
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

	printf("finito\n");
	
	return 1;
	
}





