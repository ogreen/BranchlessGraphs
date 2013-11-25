#pragma once
#include <stdint.h>

void BFSSeq(uint32_t* off, uint32_t* ind, uint32_t* Queue, uint32_t* level,uint32_t currRoot);
void BFSSeqBranchless(uint32_t* off, uint32_t* ind, uint32_t* Queue, uint32_t* level, uint32_t currRoot);
void BFSSeqBranchlessAsm(uint32_t* off, uint32_t* ind, uint32_t* Queue, uint32_t* level, uint32_t currRoot);
void BFSSeqBranchlessSSE(uint32_t* off, uint32_t* ind, uint32_t* Queue, uint32_t* level, uint32_t currRoot);
void BFSSeqBranchlessMICPartVec(uint32_t* off, uint32_t* ind, uint32_t* Queue, uint32_t* level, uint32_t currRoot);
void BFSSeqBranchlessMICFullVec(uint32_t* off, uint32_t* ind, uint32_t* Queue, uint32_t* level, uint32_t currRoot);
