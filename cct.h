#pragma once

#include <inttypes.h>

void benchMarkCCT (const int32_t nv, const int32_t ne, const int32_t * off,    const int32_t * ind, int32_t * triNE, int32_t* allTriangles,char* fileName,int32_t benchMarkSyn, int32_t synSize);


void triangleCountBranchBased (const int32_t nv, const int32_t ne, const int32_t * off,    const int32_t * ind, int32_t * triNE, int32_t* allTriangles,char* fileName);
void triangleCountBranchAvoiding (const int32_t nv, const int32_t ne,const int32_t * off, const int32_t * ind, int32_t * triNE, int32_t* allTriangles,char* graphName);


