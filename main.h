#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

void BFSSeq(uint32_t* off, uint32_t* ind, uint32_t* Queue, uint32_t* level, uint32_t currRoot);
void BFSSeqBranchless(uint32_t* off, uint32_t* ind, uint32_t* Queue, uint32_t* level, uint32_t currRoot);
void BFSSeqBranchlessAsm(uint32_t* off, uint32_t* ind, uint32_t* Queue, uint32_t* level, uint32_t currRoot);
void BFSSeqBranchlessSSE(uint32_t* off, uint32_t* ind, uint32_t* Queue, uint32_t* level, uint32_t currRoot);
void BFSSeqBranchlessAVX2(uint32_t* off, uint32_t* ind, uint32_t* Queue, uint32_t* level, uint32_t currRoot);
void BFSSeqBranchlessMICPartVec(uint32_t* off, uint32_t* ind, uint32_t* Queue, uint32_t* level, uint32_t currRoot);
void BFSSeqBranchlessMICFullVec(uint32_t* off, uint32_t* ind, uint32_t* Queue, uint32_t* level, uint32_t currRoot);

bool SVSeq(size_t nv, uint32_t* component_map, uint32_t* off, uint32_t* ind);
bool SVBranchless(size_t nv, uint32_t* component_map, uint32_t* off, uint32_t* ind);
bool SVBranchlessAsm(size_t nv, uint32_t* component_map, uint32_t* off, uint32_t* ind);
bool SVBranchlessSSE4_1(size_t nv, uint32_t* component_map, uint32_t* off, uint32_t* ind);
bool SVBranchlessMIC(size_t nv, uint32_t* component_map, uint32_t* off, uint32_t* ind);
