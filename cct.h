#pragma once

#include <inttypes.h>

void triangleCountBranchBased (const int32_t nv, const int32_t * off,
    const int32_t * ind, int64_t * triNE,
    int64_t* allTriangles);


void triangleCountBranchAvoiding (const int32_t nv, const int32_t * off,
    const int32_t * ind, int64_t * triNE,
    int64_t* allTriangles);

