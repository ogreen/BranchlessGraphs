#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef void (*BFSFunction)(uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t* level, uint32_t currRoot);
void BFSSeq(uint32_t* off, uint32_t* ind, uint32_t* Queue, uint32_t* level, uint32_t currRoot);
void BFSSeqBranchless(uint32_t* off, uint32_t* ind, uint32_t* Queue, uint32_t* level, uint32_t currRoot);
void BFSSeqBranchlessAsm(uint32_t* off, uint32_t* ind, uint32_t* Queue, uint32_t* level, uint32_t currRoot);
void BFSSeqBranchlessSSE4_1(uint32_t* off, uint32_t* ind, uint32_t* Queue, uint32_t* level, uint32_t currRoot);
void BFSSeqBranchlessAVX2(uint32_t* off, uint32_t* ind, uint32_t* Queue, uint32_t* level, uint32_t currRoot);
void BFSSeqBranchlessMIC(uint32_t* off, uint32_t* ind, uint32_t* Queue, uint32_t* level, uint32_t currRoot);

typedef bool (*SVFunction)(size_t nv, uint32_t* component_map, uint32_t* off, uint32_t* ind);
bool SVSeq(size_t nv, uint32_t* component_map, uint32_t* off, uint32_t* ind);
bool SVBranchless(size_t nv, uint32_t* component_map, uint32_t* off, uint32_t* ind);
bool SVBranchlessAsm(size_t nv, uint32_t* component_map, uint32_t* off, uint32_t* ind);
bool SVBranchlessSSE4_1(size_t nv, uint32_t* component_map, uint32_t* off, uint32_t* ind);
bool SVBranchlessMIC(size_t nv, uint32_t* component_map, uint32_t* off, uint32_t* ind);


typedef uint32_t (*BFSBUFunction)(uint32_t* off, uint32_t* ind, uint32_t* bitmap, uint32_t* level, uint32_t currRoot, uint32_t nv,uint32_t currLevel,uint32_t nextLevel);
uint32_t BFSSeqBU(uint32_t* off, uint32_t* ind, uint32_t* bitmap, uint32_t* level, uint32_t currRoot, uint32_t nv,uint32_t currLevel,uint32_t nextLevel);

