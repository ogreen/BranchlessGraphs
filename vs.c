/* -*- mode: C; mode: folding; fill-column: 70; -*- */
#define _XOPEN_SOURCE 600
#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS 64

#include "stinger-atomics.h"
#include "x86-full-empty.h"
#include "stinger-utils.h"
#include "stinger.h"
#include "timer.h"
#include "xmalloc.h"

#include "papi.h"
#include "main.h"

#define ACTI(k) (action[2*(k)])
#define ACTJ(k) (action[2*(k)+1])

#define LEVEL_IS_NEG(k) ((level[(k)] < 0) && (level[(k)] != INFINITY_MY))
#define LEVEL_IS_POS(k) ((level[(k)] >= 0) && (level[(k)] != INFINITY_MY))
#define LEVEL_IS_INF(k) (level[(k)] == INFINITY_MY)
#define LEVEL_EQUALS(k,y) ((level[(k)] == (y)) && (level[(k)] != INFINITY_MY))
#define SWAP_UINT64(x,y) {uint64_t tmp = (x); (x) = (y); (y) = tmp;}

#define INFINITY_MY 10000000
#define EMPTY_NEIGHBOR -1073741824

#define CC_DBG 1
#define CC_STATS 0
#define CSV 0
#define SHORTCUT 0

#define COUNTCC 0

#if CC_STATS
static uint64_t bfs_deletes_in_tree = 0;
static uint64_t bfs_inserts_in_tree_as_parents = 0;
static uint64_t bfs_inserts_in_tree_as_neighbors = 0;
static uint64_t bfs_inserts_in_tree_as_replacement = 0;
static uint64_t bfs_inserts_bridged = 0;
static uint64_t bfs_real_deletes = 0;
static uint64_t bfs_real_inserts = 0;
static uint64_t bfs_total_deletes = 0;
static uint64_t bfs_total_inserts = 0;
static uint64_t bfs_unsafe_deletes = 0;
#if CSV
#define CC_STAT_START(X) printf("\n%ld,", X)
#define CC_STAT_INT64(X,Y) printf("\"%s\",%ld,",X,Y)
#define CC_STAT_DOUBLE(X,Y) printf("\"%s\",%lf,",X,Y)
static char filename[300];
#else
#define CC_STAT_START(X) PRINT_STAT_INT64("stats_start", X)
#define CC_STAT_INT64(X,Y) PRINT_STAT_INT64(X,Y)
#define CC_STAT_DOUBLE(X,Y) PRINT_STAT_DOUBLE(X,Y)
#endif
#define CC_STAT(X) X
#else
#define CC_STAT_START(X)
#define CC_STAT_INT64(X,Y) 
#define CC_STAT_DOUBLE(X,Y) 
#define CC_STAT(X) 
#endif

#if CC_DBG
#define PRINT_HERE() \
	printf("\n* %s - %d", __func__, __LINE__); fflush(stdout);
#else
#define PRINT_HERE() 
#endif

#define LINE_SIZE 100000








int64_t psv_regular (int64_t nv,int64_t * component_map, int64_t* off, int64_t* ind,int64_t* iterations)
{
	*iterations=0;
	OMP ("omp parallel for")
		for (uint64_t i = 0; i < nv; i++) {
			component_map[i] = i;
		}
	int32_t iter=0;
	double time_iter[40];


	while (1) {
		int changed = 0;
		(*iterations)++;

		for(int64_t v=0;v<nv;v++){

			const int64_t *restrict vind = &ind[off[v]];
			const size_t vdeg = off[v + 1] - off[v];

			for(int64_t edge=0; edge<vdeg;edge++){		
				const int64_t u = vind[edge];
				if (component_map[u] < component_map[v]) {
					component_map[v] = component_map[u];
					changed++;
				}
			}
		}

		if (!changed)
			break;

		//    OMP ("omp parallel for")
		//      MTA ("mta assert nodep")
#if SHORTCUT
		for (uint64_t i = 0; i < nv; i++) {
			while (component_map[i] != component_map[component_map[i]])
				component_map[i] = component_map[component_map[i]];
			}
#endif

	}

	uint64_t components = 1;
	return components;

#if COUNTCC
	MTA ("mta assert nodep")
		OMP ("omp parallel for reduction(+:components)")
		for (uint64_t i = 1; i < nv; i++) {
			if (component_map[i] == i) {
				components++;
			}
		}
#endif



	}





int64_t psv_time_each_iteration (int64_t nv,int64_t * component_map, int64_t* off, int64_t* ind, bpIteration** bpIt)
{
	OMP ("omp parallel for")
		for (uint64_t i = 0; i < nv; i++) {
			component_map[i] = i;
		}
	int32_t iter=0;
	double time_iter[1000];


	while (1) {
		tic();
		int changed = 0;

		for(int64_t v=0;v<nv;v++){

			const int64_t *restrict vind = &ind[off[v]];
			const size_t vdeg = off[v + 1] - off[v];

			for(int64_t edge=0; edge<vdeg;edge++){		
				const int64_t u = vind[edge];
				if (component_map[u] < component_map[v]) {
					component_map[v] = component_map[u];
					changed++;
				}
			}
		}

		if (!changed)
			break;

		//    OMP ("omp parallel for")
		//      MTA ("mta assert nodep")
#if SHORTCUT
		for (uint64_t i = 0; i < nv; i++) {
			while (component_map[i] != component_map[component_map[i]])
				component_map[i] = component_map[component_map[i]];
			}
#endif

		bpIt[iter++]->time=toc();

	}
	bpIt[iter++]->time=toc();	


//	printf("\n");
//	for(int32_t it=0; it<iter;it++)
//		printf("%lf\n",time_iter[it]);


	uint64_t components = 1;
	return components;


#if COUNTCC
	MTA ("mta assert nodep")
		OMP ("omp parallel for reduction(+:components)")
		for (uint64_t i = 1; i < nv; i++) {
			if (component_map[i] == i) {
				components++;
			}
		}
#endif

}


int64_t psv_count_swap (int64_t nv,int64_t * component_map, int64_t* off, int64_t* ind, bpIteration** bpIt)
{
	OMP ("omp parallel for")
		for (uint64_t i = 0; i < nv; i++) {
			component_map[i] = i;
		}
	int32_t iter=0;
	int64_t swap[1000];
	int64_t mP[1000];
	int64_t hits[1000];
	int32_t ST,WT,SNT,WNT;
	ST=WT=SNT=WNT=0;
   WT=1;

	while (1) {

		int changed = 0;

		int64_t loopSwap=0,miss=0,hitPred=0,branchCounter=0;
		// A=0,B=1
		int32_t curr=0,prev=1, prev_prev=0;	

 

		for(int64_t v=0;v<nv;v++){

			const int64_t *restrict vind = &ind[off[v]];
			const size_t vdeg = off[v + 1] - off[v];

			for(int64_t edge=0; edge<vdeg;edge++){		
				const int64_t u = vind[edge];
				branchCounter++;
				if (component_map[u] < component_map[v]) 
				{
					component_map[v] = component_map[u];
					changed++;

					curr=0;
					if(curr!=prev&&prev!=prev_prev)
					{	
						loopSwap++;
					}

				    if(WT==1)
					{
					  WT=0;
					  ST=1;
					}
					else if(SNT==1)
					{
					  SNT=0;
					  WNT=1;
					  miss++;
					}
					else if(WNT==1)
					{
					  WNT=0;
					  WT=1;
					  miss++;
					}



					prev_prev=prev;
					prev=curr;


				}
				else
				{
					curr=1;
					if(curr!=prev&&prev!=prev_prev)
					{	
						loopSwap++;
					}

				    if(WT==1)
					{
					  WT=0;
					  WNT=1;
					  miss++;
					}
					else if(ST==1)
					{
					  ST=0;
					  WT=1;
					  miss++;
					}
					else if(WNT==1)
					{
					  WNT=0;
					  SNT=1;
					}

					prev_prev=prev;			
					prev=curr;	
				}
			}
		}

		bpIt[iter]->twoStateBranches = miss; 
		bpIt[iter]->twoStateSwap = loopSwap;
		bpIt[iter]->loopIterations=bpIt[iter]->twoStateBranches=branchCounter;
				
		iter++;
		
		if (!changed)
			break;

		//    OMP ("omp parallel for")
		//      MTA ("mta assert nodep")
#if SHORTCUT
		for (uint64_t i = 0; i < nv; i++) {
			while (component_map[i] != component_map[component_map[i]])
				component_map[i] = component_map[component_map[i]];
			}
#endif


	}

//	printf("\n");
//	for(int32_t it=0; it<iter;it++)
//		printf("%ld , %ld , %ld\n",swap[it],mP[it], hits[it]);


	uint64_t components = 1;
	return components;


#if COUNTCC
	MTA ("mta assert nodep")
		OMP ("omp parallel for reduction(+:components)")
		for (uint64_t i = 1; i < nv; i++) {
			if (component_map[i] == i) {
				components++;
			}
		}
#endif



}



int64_t psv_papi (int64_t nv,int64_t * component_map, int64_t* off, int64_t* ind, bpIteration** bpIt)
{
	PAPI_event_info_t info;
	int i, j, retval;
	int TESTS_QUIET=0;

	double x = 1.1, y;
	long long t1, t2;
	int nevents = PAPI_EVENTS;
	int eventset = PAPI_NULL;
	int events[PAPI_EVENTS];
	long long values[PAPI_EVENTS];

	int c=0;
	events[c++] = PAPI_BR_NTK;
	events[c++]=PAPI_BR_TKN; 

	events[c++]=PAPI_BR_CN; 
	events[c++]=PAPI_BR_UCN;

	events[c++] = PAPI_BR_PRC;
	events[c++] = PAPI_BR_INS;
	events[c++] = PAPI_BR_MSP;



	retval = PAPI_library_init( PAPI_VER_CURRENT );
	retval = PAPI_create_eventset( &eventset );

	nevents = 0;

	for ( i = 0; i < PAPI_EVENTS; i++ ) {
		if ( PAPI_query_event( events[i] ) != PAPI_OK )
			continue;
		if ( PAPI_add_event( eventset, events[i] ) == PAPI_OK ) {
			events[nevents] = events[i];
			nevents++;

		}
	}

	int32_t iter=0;

	OMP ("omp parallel for")
		for (uint64_t i = 0; i < nv; i++) {
			component_map[i] = i;
		}
	
	while (1) {
		int changed = 0;
		
		for ( i = 0; i < PAPI_EVENTS; i++ ) {
			bpIt[iter]->papiValues[i] = 0;
		}		
	
	retval = PAPI_start( eventset );


		for(int64_t v=0;v<nv;v++){

			const int64_t *restrict vind = &ind[off[v]];
			const size_t vdeg = off[v + 1] - off[v];

			for(int64_t edge=0; edge<vdeg;edge++){		
				const int64_t u = vind[edge];
				if (component_map[u] < component_map[v]) {
					component_map[v] = component_map[u];
					changed++;
				}
			}
		}
		


	retval = PAPI_stop( eventset, values );
		
		if (!changed)
			break;

		//    OMP ("omp parallel for")
		//      MTA ("mta assert nodep")
#if SHORTCUT
		for (uint64_t i = 0; i < nv; i++) {
			while (component_map[i] != component_map[component_map[i]])
				component_map[i] = component_map[component_map[i]];
			}
#endif

		for ( i = 0; i < PAPI_EVENTS; i++ ) {
//			printf("%lld,\n", values[i]);
			bpIt[iter]->papiValues[i]=values[i];
		}		

		iter++;

	}

	for ( i = 0; i < PAPI_EVENTS; i++ ) {
//		printf("%lld,\n", values[i]);
		bpIt[iter]->papiValues[i]=values[i];
	}		

		iter++;

	retval = PAPI_remove_events( eventset, events, nevents );
	retval = PAPI_destroy_eventset( &eventset );
	eventset = PAPI_NULL;
	retval = PAPI_create_eventset( &eventset );

	uint64_t components = 1;
	return components;
	
	

#if COUNTCC
	MTA ("mta assert nodep")
		OMP ("omp parallel for reduction(+:components)")
		for (uint64_t i = 1; i < nv; i++) {
			if (component_map[i] == i) {
				components++;
			}
		}
#endif



}






















































int64_t parallel_shiloach_vishkin_components_faster (int64_t nv,int64_t * component_map, int64_t* off, int64_t* ind, int64_t* vertexFlag, int64_t* vertexFlagNext)
{
	OMP ("omp parallel for")
		for (uint64_t i = 0; i < nv; i++) {
			component_map[i] = i;
			vertexFlag[i]=0;
			vertexFlagNext[i]=1;
		}
	int itCounter=0;
	while (1) {
		itCounter++;
		//  if(itCounter>4)
		//  break;
		int changed = 0;
		int marked=0;
		int64_t edges =0;
		for(int64_t v=0;v<nv;v++){
			if(!vertexFlagNext[v])
				continue;
			const int64_t *restrict vind = &ind[off[v]];
			const size_t vdeg = off[v + 1] - off[v];

			vertexFlag[v]=1;
			edges+=vdeg;
			for(int64_t edge=0; edge<vdeg;edge++){
				const int64_t u = vind[edge];

				vertexFlag[u]=1;
				marked++;
			}			
		}	  

		for(int64_t v=0;v<nv;v++){
			if(!vertexFlag[v])
				continue;
			vertexFlag[v]=0;
			vertexFlagNext[v]=0;
			const int64_t *restrict vind = &ind[off[v]];
			const size_t vdeg = off[v + 1] - off[v];

			for(int64_t edge=0; edge<vdeg;edge++){
				const int64_t u = vind[edge];

				if (component_map[u] <
						component_map[v]) {
					component_map[v] = component_map[u];
					changed++;
					vertexFlagNext[v]=1;
					//			vertexFlag[v]=1;
				}
			}
		}
		//	printf("%ld\n", changed);
		//	printf("\n%ld  ,  %ld  ,  %ld", marked, changed, edges);


		if (!changed)
			break;
		//	continue;

		//    OMP ("omp parallel for")
		//      MTA ("mta assert nodep")
		      for (uint64_t i = 0; i < nv; i++) {
			while (component_map[i] != component_map[component_map[i]])
			component_map[i] = component_map[component_map[i]];
			}
		   }



		uint64_t components = 1;
		return components;


		/* Count components */
		MTA ("mta assert nodep")
			OMP ("omp parallel for reduction(+:components)")
			for (uint64_t i = 1; i < nv; i++) {
				if (component_map[i] == i) {
					components++;
				}
			}


}


int64_t parallel_shiloach_vishkin_components_branchless (int64_t nv,int64_t * component_map, int64_t* off, int64_t* ind)
	{

		OMP ("omp parallel for")
			for (uint64_t i = 0; i < nv; i++) {
				component_map[i] = i;
			}
		uint64_t flag;
		while (1) {
			int64_t changed = 0;

			for(int64_t v=0;v<nv;v++){

				const int64_t *restrict vind = &ind[off[v]];
				const size_t vdeg = off[v + 1] - off[v];

				for(int64_t edge=0; edge<vdeg;edge++){		
					const int64_t u = vind[edge];
					flag = (uint64_t)((int64_t)component_map[u]-(int64_t)component_map[v]) >> 63;
					//	printf("%ld,", flag);
					component_map[v]+=(1-flag)*(component_map[u]-component_map[v]);		
					//		component_map[v]=(1-flag)*component_map[u]+(flag)*component_map[v];
					//		flag = (uint32_t)(component_map[v]- component_map[u]) >> 31;
					//		component_map[v]+=(flag)*(component_map[u]-component_map[v]);		


					changed += flag;

				}
			}
			//    STINGER_PARALLEL_FORALL_EDGES_END ();

			//	printf("%ld\n", changed);
			if (!changed)
				break;

			//    OMP ("omp parallel for")
			//      MTA ("mta assert nodep")
			for (uint64_t i = 0; i < nv; i++) {
				while (component_map[i] != component_map[component_map[i]])
					component_map[i] = component_map[component_map[i]];
			}

		}
	uint64_t components = 1;

	return components;


		MTA ("mta assert nodep")
			OMP ("omp parallel for reduction(+:components)")
			for (uint64_t i = 1; i < nv; i++) {
				if (component_map[i] == i) {
					components++;
				}
			}

}










		/*

		   void BFSSeq(int64_t* off, int64_t* ind,
		   int64_t* Queue, int64_t* level,int64_t currRoot)
		   {
		   level[currRoot] = 0;


		   Queue[0] = currRoot;
		   level[currRoot]=0;
		   int64_t qStart=0,qEnd=1;


		// While queue is not empty
		while(qStart!=qEnd)
		{
		int64_t currElement = Queue[qStart];
		int64_t k, edge;
		qStart++;

		int64_t low=off[currElement], high=off[currElement+1];
		int counter=0;
		for(edge=low; edge<high;edge++)
		{
		counter++; if (counter>10) return;
		k=ind[edge];

		printf("Levels %ld %ld %ld %ld\n",currElement,k,level[currElement],level[k]);


		// Checking if "k" has been found.
		if(level[k]==INFINITY_MY)
		{
		level[k] = level[currElement]+1;
		Queue[qEnd++] = k;
		}
		}

		}

		}



		void BFSSeqBranchless(int64_t* off, int64_t* ind,
		int64_t* Queue, int64_t* level,int64_t currRoot)
		{
		level[currRoot] = 0;


		Queue[0] = currRoot;
		level[currRoot]=0;
		int64_t qStart=0;
		int32_t flag=1;
		int32_t isINF=1;
		int64_t pre=0;
		int64_t qEnd=1;

		printf("\n%ld \n",qEnd);

		while(qStart<=qEnd)
		{
		int64_t currElement = Queue[qStart];
		int64_t k, edge;
		qStart++;

		int64_t low=off[currElement], high=off[currElement+1];		
		int counter=0;
		for(edge=low; edge<high;edge++)
		{
		counter++;
		k=ind[edge];

		// Checking if "k" has been found.
		pre = level[k];
		printf("Levels %ld %ld %ld %ld\n",currElement,k,level[currElement],level[k]);
		flag = (uint32_t)(level[currElement]-level[k]) >> 31;
		if (flag<0)
			flag=1;
		level[k]=flag*(level[currElement]+1);
		isINF = (uint32_t)(level[k]-pre)>>31;
		if (isINF <0)
			isINF=1;
		Queue[qEnd]=isINF*k;
		//				printf("\n%ld %ld \n",qEnd,Queue[qEnd]);


		qEnd+=flag*isINF;
		//			if(level[currElement]==0)
		{
			//				printf("%ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld\n",k,flag,isINF,pre,level[k],qEnd,flag*isINF, Queue[qEnd]);fflush(stdout);
		}
		//			if(qEnd%1000==0) {printf("%ld,",qEnd);fflush(stdout);}
		if (counter==10)
			return;
	}
	return;
	}

	}
	*/


