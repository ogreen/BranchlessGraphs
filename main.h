#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

void BFS_TopDown_Branchy_LevelInformation(uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t* level, uint32_t currRoot, uint32_t* edgesTraversed, uint32_t* queueStartPosition);

typedef uint32_t (*BFS_TopDown_Function)(uint32_t* off, uint32_t* ind, const uint32_t* inputQueue, uint32_t inputVerteces, uint32_t* outputQueue, uint32_t outputCapacity, uint32_t* level, uint32_t currentLevel);

uint32_t _BFS_TopDown_Branchy_CortexA15(uint32_t* vertexEdges, uint32_t* neighbors, const uint32_t* inputQueue, uint32_t inputVertices, uint32_t* outputQueue, uint32_t outputCapacity, uint32_t* levels, uint32_t currentLevel);
uint32_t _BFS_TopDown_Branchless_CortexA15(uint32_t* vertexEdges, uint32_t* neighbors, const uint32_t* inputQueue, uint32_t inputVertices, uint32_t* outputQueue, uint32_t outputCapacity, uint32_t* levels, uint32_t currentLevel);

uint32_t BFS_TopDown_Branchy(uint32_t* off, uint32_t* ind, const uint32_t* inputQueue, uint32_t inputVerteces, uint32_t* outputQueue, uint32_t* level, uint32_t currentLevel);
uint32_t BFS_TopDown_Branchless(uint32_t* off, uint32_t* ind, const uint32_t* inputQueue, uint32_t inputVerteces, uint32_t* outputQueue, uint32_t* level, uint32_t currentLevel);
uint32_t BFS_TopDown_Branchless_CMOV(uint32_t* off, uint32_t* ind, const uint32_t* inputQueue, uint32_t inputVerteces, uint32_t* outputQueue, uint32_t* level, uint32_t currentLevel);
uint32_t BFS_TopDown_Branchless_SSE4_1(uint32_t* off, uint32_t* ind, const uint32_t* inputQueue, uint32_t inputVerteces, uint32_t* outputQueue, uint32_t* level, uint32_t currentLevel);
void BFS_TopDown_Branchless_AVX2(uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t* level, uint32_t currRoot);
void BFS_TopDown_Branchless_MIC(uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t* level, uint32_t currRoot);

typedef bool (*ConnectedComponents_SV_Function)(size_t nv, uint32_t* component_map, uint32_t* off, uint32_t* ind);
bool ConnectedComponents_SV_Branchy(size_t nv, uint32_t* component_map, uint32_t* off, uint32_t* ind);
bool ConnectedComponents_SV_Branchless(size_t nv, uint32_t* component_map, uint32_t* off, uint32_t* ind);
bool ConnectedComponents_SV_Branchless_CMOV(size_t nv, uint32_t* component_map, uint32_t* off, uint32_t* ind);
bool ConnectedComponents_SV_Branchless_SSE4_1(size_t nv, uint32_t* component_map, uint32_t* off, uint32_t* ind);
bool ConnectedComponents_SV_Branchless_MIC(size_t nv, uint32_t* component_map, uint32_t* off, uint32_t* ind);

uint32_t BFS_BottomUp_Branchy_LevelInformation(uint32_t* off, uint32_t* ind, uint32_t* bitmap, uint32_t* level, uint32_t currRoot, uint32_t nv,uint32_t currLevel, uint32_t* edgesTraversed, uint32_t* verticesFound);

typedef uint32_t (*BFS_BottomUp_Function)(uint32_t* off, uint32_t* ind, uint32_t* bitmap, uint32_t* level, uint32_t nv, uint32_t currLevel);
uint32_t BFS_BottomUp_Branchy(uint32_t* off, uint32_t* ind, uint32_t* bitmap, uint32_t* level, uint32_t nv, uint32_t currLevel);
uint32_t BFS_BottomUp_Branchless(uint32_t* off, uint32_t* ind, uint32_t* bitmap, uint32_t* level, uint32_t nv, uint32_t currLevel);
uint32_t BFS_BottomUp_Branchless_CMOV(uint32_t* off, uint32_t* ind, uint32_t* bitmap, uint32_t* level, uint32_t nv, uint32_t currLevel);
