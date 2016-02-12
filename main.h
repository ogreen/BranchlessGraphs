#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>

#include "timer.h"

#define PERF_TYPE_TIME PERF_TYPE_MAX

struct PerformanceCounter {
  const char* name;
  uint32_t type;
  uint32_t subtype;
  bool supported;
};

int perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags);



//---------------------
// BFS
//---------------------

void BFS_TopDown_Branchy_LevelInformation(uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t* level, uint32_t currRoot, uint32_t* edgesTraversed, uint32_t* queueStartPosition);

typedef uint32_t (*BFS_TopDown_Function)(uint32_t* off, uint32_t* ind, const uint32_t* inputQueue, uint32_t inputVerteces, uint32_t* outputQueue, uint32_t* level, uint32_t currentLevel);

uint32_t BFS_TopDown_Branchy_PeachPy(uint32_t* vertexEdges, uint32_t* neighbors, const uint32_t* inputQueue, uint32_t inputVertices, uint32_t* outputQueue, uint32_t* levels, uint32_t currentLevel);
uint32_t BFS_TopDown_Branchless_PeachPy(uint32_t* vertexEdges, uint32_t* neighbors, const uint32_t* inputQueue, uint32_t inputVertices, uint32_t* outputQueue, uint32_t* levels, uint32_t currentLevel);
uint32_t BFS_TopDown_Branchless_Trace_PeachPy(uint32_t* vertexEdges, uint32_t* neighbors, const uint32_t* inputQueue, uint32_t inputVertices, uint32_t* outputQueue, uint32_t* levels, uint32_t currentLevel);
uint32_t BFS_TopDown_Branchlessless_PeachPy(uint32_t* vertexEdges, uint32_t* neighbors, const uint32_t* inputQueue, uint32_t inputVertices, uint32_t* outputQueue, uint32_t* levels, uint32_t currentLevel);

uint32_t BFS_TopDown_Branchless_SSE4_1(uint32_t* off, uint32_t* ind, const uint32_t* inputQueue, uint32_t inputVerteces, uint32_t* outputQueue, uint32_t* level, uint32_t currentLevel);
void BFS_TopDown_Branchless_AVX2(uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t* level, uint32_t currRoot);
void BFS_TopDown_Branchless_MIC(uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t* level, uint32_t currRoot);

//---------------------
// BC
//---------------------
 
 typedef struct {
  uint32_t* level;
  uint32_t* sigma;
  float* delta;
 } bcSOA;

 typedef struct {
  uint32_t level;
  uint32_t sigma;
  float delta;
  // uint32_t dummy;   
 }bcAOS;


typedef uint32_t (*BC_TRAV_Function)(uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t inputStart, uint32_t inputNum, 
		uint32_t outputStart, void* bcSOAAOSStruct);//uint32_t* level,uint32_t* sigma, float*delta);

uint32_t bcTreeBranchBasedSOA(uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t inputStart, uint32_t inputNum, 
		uint32_t outputStart, void* bcSOAStruct);// uint32_t* level,uint32_t* sigma, float*delta);
uint32_t bcTreeBranchAvoidingSOA(uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t inputStart, uint32_t inputNum, 
		uint32_t outputStart, void* bcSOAStruct);//uint32_t* level,uint32_t* sigma, float*delta);

uint32_t bcTreeBranchBasedAOS(uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t inputStart, uint32_t inputNum, 
    uint32_t outputStart, void* bcAOSStruct);// uint32_t* level,uint32_t* sigma, float*delta);
uint32_t bcTreeBranchAvoidingAOS(uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t inputStart, uint32_t inputNum, 
    uint32_t outputStart, void* bcAOSStruct);//uint32_t* level,uint32_t* sigma, float*delta);



typedef void (*BC_DEP_Function)(uint32_t currRoot,uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t reverseStart, uint32_t numElements, 
    void* bcSOAAOSStruct, float* totalBC);

void bcDependencyBranchBasedSOA(uint32_t currRoot,uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t reverseStart, uint32_t numElements, 
    void* bcSOAStruct, float* totalBC);
void bcDependencyBranchAvoidingSOA(uint32_t currRoot,uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t reverseStart, uint32_t numElements, 
    void* bcSOAStruct, float* totalBC);

void bcDependencyBranchBasedAOS(uint32_t currRoot,uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t reverseStart, uint32_t numElements, 
    void* bcAOSStruct, float* totalBC);
void bcDependencyBranchAvoidingAOS(uint32_t currRoot,uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t reverseStart, uint32_t numElements, 
    void* bcAOSStruct, float* totalBC);

#if defined(ARMASM)
void bcDependencyBranchAvoidingSOAOpt(uint32_t currRoot,uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t reverseStart, uint32_t numElements, 
    void* bcSOAStruct, float* totalBC);
 
void bcDependencyBranchAvoidingAOSOpt(uint32_t currRoot,uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t reverseStart, uint32_t numElements, 
    void* bcAOSStruct, float* totalBC);
#endif 

//---------------------
// Connected Components
//---------------------

typedef enum{
  SV_ALG_BRANCH_BASED=0,
  SV_ALG_BRANCH_AVOIDING,
  SV_NUMBER_OF_ALGS
} eSV_Alg;

typedef struct {
  int32_t iterationSwap;  
  float   toleranceGradient;
  float	  toleranceSteadyState;
} svControlParams;



typedef bool (*ConnectedComponents_SV_Function)(size_t vertexCount, uint32_t* componentMap, uint32_t* vertexEdges, uint32_t* neighbors);

bool ConnectedComponents_SV_Branchy_PeachPy(size_t vertexCount, uint32_t* componentMap, uint32_t* vertexEdges, uint32_t* neighbors);
bool ConnectedComponents_SV_Branchless_PeachPy(size_t vertexCount, uint32_t* componentMap, uint32_t* vertexEdges, uint32_t* neighbors);

bool ConnectedComponents_SV_Branchless_SSE4_1(size_t vertexCount, uint32_t* componentMap, uint32_t* vertexEdges, uint32_t* neighbors);
bool ConnectedComponents_SV_Branchless_MIC(size_t vertexCount, uint32_t* componentMap, uint32_t* vertexEdges, uint32_t* neighbors);

uint32_t BFS_BottomUp_Branchy_LevelInformation(uint32_t* off, uint32_t* ind, uint32_t* bitmap, uint32_t* level, uint32_t currRoot, uint32_t nv,uint32_t currLevel, uint32_t* edgesTraversed, uint32_t* verticesFound);

typedef uint32_t (*BFS_BottomUp_Function)(uint32_t* off, uint32_t* ind, uint32_t* bitmap, uint32_t* level, uint32_t nv, uint32_t currLevel);
uint32_t BFS_BottomUp_Branchy(uint32_t* off, uint32_t* ind, uint32_t* bitmap, uint32_t* level, uint32_t nv, uint32_t currLevel);
uint32_t BFS_BottomUp_Branchless(uint32_t* off, uint32_t* ind, uint32_t* bitmap, uint32_t* level, uint32_t nv, uint32_t currLevel);
uint32_t BFS_BottomUp_Branchless_CMOV(uint32_t* off, uint32_t* ind, uint32_t* bitmap, uint32_t* level, uint32_t nv, uint32_t currLevel);


//---------------------
// Triangle Counting
//---------------------

int32_t intersectionBranchBased ( const int32_t alen, const int32_t * a, const int32_t blen, const int32_t * b);
int32_t intersectionBranchAvoiding ( const int32_t alen, const int32_t * a,  const int32_t blen, const  int32_t * b);
int32_t intersectionBranchAvoidingArmAsm ( const int32_t alen, const int32_t * a,  const int32_t blen, const  int32_t * b);


typedef int32_t (*CCT_Function)( const int32_t alen, const int32_t * a,  const int32_t blen, const  int32_t * b);
