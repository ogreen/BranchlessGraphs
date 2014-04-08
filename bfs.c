#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#ifdef __x86_64__
	#include <x86intrin.h>
#endif

void BFS_TopDown_Branchy_LevelInformation(uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t* level, uint32_t currRoot, uint32_t* edgesTraversed, uint32_t* queueStartPosition) {
	level[currRoot] = 0;

	queue[0] = currRoot;
	uint32_t qStart = 0, qEnd = 1;

	uint32_t currLevel=0;

	queueStartPosition[0]=0;
	uint32_t travEdge=0;
	// While queue is not empty
	while (qStart != qEnd) {
		uint64_t currElement = queue[qStart];
		uint32_t startEdge = off[currElement];
		uint32_t stopEdge = off[currElement+1];
		uint32_t nextLevel = level[currElement] + 1;
		if (level[currElement] > currLevel){
			queueStartPosition[currLevel+1] = qStart;			
			travEdge = 0;
			currLevel = level[currElement];	
//			printf("curr level %ld",currLevel); fflush(stdout);					
		}
		
		for (uint32_t j = startEdge; startEdge < stopEdge; startEdge++) {
			uint32_t k = ind[startEdge];
			edgesTraversed[currLevel]++;
			// If this is a neighbor and has not been found
			if (level[k] > level[currElement]) {
				// Checking if "k" has been found.
				if (level[k] == INT32_MAX) {
					level[k] = nextLevel; //level[currElement]+1;
					queue[qEnd++] = k;
				}
			}
		}
		qStart++;
	}
	queueStartPosition[currLevel+1] = qStart;
}



uint32_t BFS_TopDown_Branchy(uint32_t* off, uint32_t* ind, const uint32_t* inputQueue, uint32_t inputVerteces, uint32_t* outputQueue, uint32_t* level, uint32_t currentLevel) {
	uint32_t* outputQueueStart = outputQueue;
	while (inputVerteces--) {
		uint32_t current_vertex = *inputQueue++;
		
		const uint32_t startEdge = off[current_vertex];
		const uint32_t stopEdge = off[current_vertex+1];

		for (uint32_t edge = startEdge; edge != stopEdge; edge++) {
			const uint32_t neighborVertex = ind[edge];

			if (level[neighborVertex] > currentLevel) {
				level[neighborVertex] = currentLevel;
				*outputQueue++ = neighborVertex;
			}
		}
	}
	return outputQueue - outputQueueStart;
}

uint32_t BFS_TopDown_Branchless(uint32_t* off, uint32_t* ind, const uint32_t* inputQueue, uint32_t inputVerteces, uint32_t* outputQueue, uint32_t* level, uint32_t currentLevel) {
	uint32_t outputIndex = 0;
	while (inputVerteces--) {
		uint32_t current_vertex = *inputQueue++;
		
		const uint32_t startEdge = off[current_vertex];
		const uint32_t stopEdge = off[current_vertex+1];

		for (uint32_t edge = startEdge; edge != stopEdge; edge++) {
			const uint32_t neighborVertex = ind[edge];
			const uint32_t levelDiff = currentLevel - level[neighborVertex];
			outputQueue[outputIndex] = neighborVertex;
			const uint32_t isFrontier = levelDiff >> 31;
			outputIndex += isFrontier;
			level[neighborVertex] += isFrontier * levelDiff;
		}
	}
	return outputIndex;
}

#if defined(__x86_64__) && !defined(__MIC__)
	uint32_t BFS_TopDown_Branchless_CMOV(uint32_t* off, uint32_t* ind, const uint32_t* inputQueue, uint32_t inputVerteces, uint32_t* outputQueue, uint32_t* level, uint32_t currentLevel) {
		uint32_t outputIndex = 0;
		while (inputVerteces--) {
			uint32_t current_vertex = *inputQueue++;
			
			const uint32_t startEdge = off[current_vertex];
			const uint32_t stopEdge = off[current_vertex+1];

			for (uint32_t edge = startEdge; edge != stopEdge; edge++) {
				const uint32_t neighborVertex = ind[edge];
				uint32_t neighborLevel = level[neighborVertex];
				outputQueue[outputIndex] = neighborVertex;
				__asm__ __volatile__ (
					"CMPL %[neighborLevel], %[currentLevel];"
					"CMOVNGEL %[currentLevel], %[neighborLevel];"
					"ADCL $0, %[outputIndex];"
					: [outputIndex] "+r" (outputIndex), [neighborLevel] "+r" (neighborLevel)
					: [currentLevel] "r" (currentLevel)
					: "cc"
				);
				level[neighborVertex] = neighborLevel;
			}
		}
		return outputIndex;
	}
#endif

/*
void BFSSeqBranchlessSSE(int64_t* off, int64_t* ind, int64_t* queue, int64_t* level, int64_t currRoot) {
	level[currRoot] = 0;

	queue[0] = currRoot;
	int64_t qStart = 0, qEnd = 1;

	const __m128i shuffleTable[4] = {
		_mm_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
		_mm_setr_epi8(   0,    1,    2,    3,    4,    5,    6,    7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
		_mm_setr_epi8(   8,    9,   10,   11,   12,   13,   14,   15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
		_mm_setr_epi8(   0,    1,    2,    3,    4,    5,    6,    7,    8,    9,   10,   11,   12,   13,   14,   15)
	};

	// While queue is not empty
	while (qStart < qEnd) {
		uint64_t currElement;
		currElement = queue[qStart];
		qStart++;

		const int64_t startEdge = off[currElement];

		const int64_t stopEdge = off[currElement + 1];
		const int64_t nextLevel = level[currElement] + 1;
		const __m128i mmNextLevel = _mm_set1_epi64x(nextLevel);
		int64_t j;
		
		j = startEdge;
		for (; j < stopEdge - 2; j += 2) {
			const int64_t k0 = ind[j];
			const int64_t k1 = ind[j + 1];
			const __m128i mmK = _mm_set_epi64x(k1, k0);
			__m128i mmLevelK = _mm_insert_epi64(_mm_loadl_epi64((__m128i*)&level[k0]), level[k1], 1);
			const __m128i predicate = _mm_cmpgt_epi64(mmLevelK, mmNextLevel);
			const unsigned qMask = _mm_movemask_pd(_mm_castsi128_pd(predicate));
			_mm_storeu_si128((__m128i*)&queue[qEnd], _mm_shuffle_epi8(mmK, shuffleTable[qMask]));
			qEnd += __builtin_popcount(qMask);
			mmLevelK = _mm_blendv_epi8(mmLevelK, mmNextLevel, predicate);
			level[k0] = _mm_cvtsi128_si64(mmLevelK);
			level[k1] = _mm_extract_epi64(mmLevelK, 1);
		}
		for (; j < stopEdge; j ++) {
			const int64_t k = ind[j];
			int64_t levelK = level[k];
			queue[qEnd] = k;
			__asm__ __volatile__ (
				"CMPQ %[levelK], %[nextLevel];"
				"CMOVNGEQ %[nextLevel], %[levelK];"
				"ADCQ $0, %[qEnd];"
				: [qEnd] "+r" (qEnd), [levelK] "+r" (levelK)
				: [nextLevel] "r" (nextLevel)
				: "cc"
			);
			level[k] = levelK;
		}

	}
}
*/

#ifdef __SSE4_1__
	uint32_t BFS_TopDown_Branchless_SSE4_1(uint32_t* off, uint32_t* ind, const uint32_t* inputQueue, uint32_t inputVerteces, uint32_t* outputQueue, uint32_t* level, uint32_t currentLevel) {
		#define _ 0x80
		const __m128i compactionTable[16] = {
			_mm_setr_epi8(  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _ ),
			_mm_setr_epi8(  0,  1,  2,  3,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _ ),
			_mm_setr_epi8(  4,  5,  6,  7,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _ ),
			_mm_setr_epi8(  0,  1,  2,  3,  4,  5,  6,  7,  _,  _,  _,  _,  _,  _,  _,  _ ),
			_mm_setr_epi8(  8,  9, 10, 11,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _ ),
			_mm_setr_epi8(  0,  1,  2,  3,  8,  9, 10, 11,  _,  _,  _,  _,  _,  _,  _,  _ ),
			_mm_setr_epi8(  4,  5,  6,  7,  8,  9, 10, 11,  _,  _,  _,  _,  _,  _,  _,  _ ),
			_mm_setr_epi8(  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11,  _,  _,  _,  _ ),
			_mm_setr_epi8( 12, 13, 14, 15,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _ ),
			_mm_setr_epi8(  0,  1,  2,  3, 12, 13, 14, 15,  _,  _,  _,  _,  _,  _,  _,  _ ),
			_mm_setr_epi8(  4,  5,  6,  7, 12, 13, 14, 15,  _,  _,  _,  _,  _,  _,  _,  _ ),
			_mm_setr_epi8(  0,  1,  2,  3,  4,  5,  6,  7, 12, 13, 14, 15,  _,  _,  _,  _ ),
			_mm_setr_epi8(  8,  9, 10, 11, 12, 13, 14, 15,  _,  _,  _,  _,  _,  _,  _,  _ ),
			_mm_setr_epi8(  0,  1,  2,  3,  8,  9, 10, 11, 12, 13, 14, 15,  _,  _,  _,  _ ),
			_mm_setr_epi8(  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  _,  _,  _,  _ ),
			_mm_setr_epi8(  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 )
		};
		#undef _
		const __m128i currentLevelX4 = _mm_set1_epi32(currentLevel);
		uint32_t outputIndex = 0;
		while (inputVerteces--) {
			uint32_t currentVertex = *inputQueue++;
			const uint32_t startEdge = off[currentVertex];
			const uint32_t stopEdge = off[currentVertex + 1];

			uint32_t edge = startEdge;
			for (; edge + 4 <= stopEdge; edge += 4) {
				const uint32_t neighborVertex0 = ind[edge];
				const uint32_t neighborVertex1 = ind[edge + 1];
				const uint32_t neighborVertex2 = ind[edge + 2];
				const uint32_t neighborVertex3 = ind[edge + 3];
				const __m128i neightborVertices = _mm_loadu_si128((__m128i*)&ind[edge]);
				__m128i neighborLevels = _mm_unpacklo_epi64(
					_mm_insert_epi32(_mm_cvtsi32_si128(level[neighborVertex0]), level[neighborVertex1], 1),
					_mm_insert_epi32(_mm_cvtsi32_si128(level[neighborVertex2]), level[neighborVertex3], 1)
				);
				__m128i frontierPredicates = _mm_cmpgt_epi32(neighborLevels, currentLevelX4);
				const uint32_t frontierMask = _mm_movemask_ps(_mm_castsi128_ps(frontierPredicates));
				const __m128i compactedNeightborVertices = _mm_shuffle_epi8(neightborVertices, compactionTable[frontierMask]);
				_mm_storeu_si128((__m128i*)&outputQueue[outputIndex], compactedNeightborVertices);

				outputIndex += __builtin_popcount(frontierMask);
				neighborLevels = _mm_min_epi32(neighborLevels, currentLevelX4);
				level[neighborVertex0] = _mm_cvtsi128_si32(neighborLevels);
				level[neighborVertex1] = _mm_extract_epi32(neighborLevels, 1);
				level[neighborVertex2] = _mm_extract_epi32(neighborLevels, 2);
				level[neighborVertex3] = _mm_extract_epi32(neighborLevels, 3);
			}
			for (; edge != stopEdge; edge++) {
				const uint32_t neighborVertex = ind[edge];
				uint32_t neighborLevel = level[neighborVertex];
				outputQueue[outputIndex] = neighborVertex;
				__asm__ __volatile__ (
					"CMPL %[neighborLevel], %[currentLevel];"
					"CMOVNGEL %[currentLevel], %[neighborLevel];"
					"ADCL $0, %[outputIndex];"
					: [outputIndex] "+r" (outputIndex), [neighborLevel] "+r" (neighborLevel)
					: [currentLevel] "r" (currentLevel)
					: "cc"
				);
				level[neighborVertex] = neighborLevel;
			}
		}
		return outputIndex;
	}

	//~ void BFS_TopDown_Branchless_SSE4_1(uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t* level, uint32_t currRoot) {
		//~ level[currRoot] = 0;

		//~ queue[0] = currRoot;
		//~ uint32_t qStart = 0, qEnd = 1;

		//~ #define _ 0x80
		//~ const __m128i shuffleTable[16] = {
			//~ _mm_setr_epi8(  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8(  0,  1,  2,  3,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8(  4,  5,  6,  7,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8(  0,  1,  2,  3,  4,  5,  6,  7,  _,  _,  _,  _,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8(  8,  9, 10, 11,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8(  0,  1,  2,  3,  8,  9, 10, 11,  _,  _,  _,  _,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8(  4,  5,  6,  7,  8,  9, 10, 11,  _,  _,  _,  _,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8(  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8( 12, 13, 14, 15,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8(  0,  1,  2,  3, 12, 13, 14, 15,  _,  _,  _,  _,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8(  4,  5,  6,  7, 12, 13, 14, 15,  _,  _,  _,  _,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8(  0,  1,  2,  3,  4,  5,  6,  7, 12, 13, 14, 15,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8(  8,  9, 10, 11, 12, 13, 14, 15,  _,  _,  _,  _,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8(  0,  1,  2,  3,  8,  9, 10, 11, 12, 13, 14, 15,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8(  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  _,  _,  _,  _ ),
			//~ _mm_setr_epi8(  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 )
		//~ };
		//~ #undef _

		//~ // While queue is not empty
		//~ while (qStart < qEnd) {
			//~ uint32_t currElement = queue[qStart];
			//~ qStart++;

			//~ const uint32_t startEdge = off[currElement];

			//~ const uint32_t stopEdge = off[currElement + 1];
			//~ const uint32_t nextLevel = level[currElement] + 1;
			//~ const __m128i mmNextLevel = _mm_set1_epi32(nextLevel);
			
			//~ uint32_t j = startEdge;
			//~ for (; j + 4 < stopEdge; j += 4) {
				//~ const uint32_t k0 = ind[j];
				//~ const uint32_t k1 = ind[j + 1];
				//~ const uint32_t k2 = ind[j + 2];
				//~ const uint32_t k3 = ind[j + 3];
				//~ const __m128i mmK = _mm_loadu_si128((__m128i*)&ind[j]);
				//~ __m128i mmLevelK = _mm_insert_epi32(_mm_insert_epi32(_mm_insert_epi32(_mm_cvtsi32_si128(level[k0]), level[k1], 1), level[k2], 2), level[k3], 3);
				//~ __m128i predicate = _mm_cmpgt_epi32(mmLevelK, mmNextLevel);
				//~ const unsigned qMask = _mm_movemask_ps(_mm_castsi128_ps(predicate));
				//~ //const unsigned qMask = _mm_movemask_epi8(predicate);

				//~ //predicate = _mm_and_si128(predicate, _mm_set1_epi8(4));
				//~ //__m128i mask = predicate;
				//~ /*
				//~ mask = _mm_add_epi8(mask, _mm_srli_si128(predicate, 4));
				//~ mask = _mm_add_epi8(mask, _mm_srli_si128(predicate, 8));
				//~ mask = _mm_add_epi8(mask, _mm_srli_si128(predicate, 12));
				//~ */
				//~ //~ mask = _mm_add_epi8(mask, _mm_srli_si128(mask, 4));
				//~ //~ mask = _mm_add_epi8(mask, _mm_srli_si128(mask, 8));
				//~ //~ mask = _mm_add_epi8(mask, _mm_setr_epi8(0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3));
				//~ //const __m128i compressed = _mm_shuffle_epi8(mmK, mask);
				//~ const __m128i compressed = _mm_shuffle_epi8(mmK, shuffleTable[qMask]);
				//~ _mm_storeu_si128((__m128i*)&queue[qEnd], compressed);

				//~ qEnd += __builtin_popcount(qMask);
				//~ //~ qEnd += __builtin_popcount(qMask) >> 2;
				//~ mmLevelK = _mm_min_epi32(mmLevelK, mmNextLevel);
				//~ level[k0] = _mm_cvtsi128_si32(mmLevelK);
				//~ level[k1] = _mm_extract_epi32(mmLevelK, 1);
				//~ level[k2] = _mm_extract_epi32(mmLevelK, 2);
				//~ level[k3] = _mm_extract_epi32(mmLevelK, 3);
			//~ }
			//~ for (; j < stopEdge; j ++) {
				//~ const uint32_t k = ind[j];
				//~ uint32_t levelK = level[k];
				//~ queue[qEnd] = k;
				//~ __asm__ __volatile__ (
					//~ "CMPL %[levelK], %[nextLevel];"
					//~ "CMOVNGEL %[nextLevel], %[levelK];"
					//~ "ADCL $0, %[qEnd];"
					//~ : [qEnd] "+r" (qEnd), [levelK] "+r" (levelK)
					//~ : [nextLevel] "r" (nextLevel)
					//~ : "cc"
				//~ );
				//~ level[k] = levelK;
			//~ }

		//~ }
	//~ }
#endif

#ifdef __AVX2__
	void BFS_TopDown_Branchless_AVX2(uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t* level, uint32_t currRoot) {
		level[currRoot] = 0;

		queue[0] = currRoot;
		uint32_t qStart = 0, qEnd = 1;

		const __m256i shuffleTable[256] = {
			_mm256_setr_epi32( 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    3, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    4, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    4, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    4, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    4, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    4, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    4, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    4, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    4, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    3,    4, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    4, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    4, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    4, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3,    4, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    4, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    4, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    4, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    5, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    5, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    5, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    5, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    5, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    5, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    5, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    5, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    3,    5, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    5, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    5, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    5, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3,    5, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    5, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    5, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    5, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    4,    5, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    4,    5, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    4,    5, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    4,    5, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    4,    5, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    4,    5, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    4,    5, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    4,    5, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    3,    4,    5, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    4,    5, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    4,    5, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    4,    5, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3,    4,    5, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    4,    5, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    4,    5, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    4,    5, 0x80, 0x80),
			_mm256_setr_epi32(    6, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    6, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    6, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    6, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    3,    6, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    4,    6, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    4,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    4,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    4,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    4,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    4,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    4,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    4,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    3,    4,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    4,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    4,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    4,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3,    4,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    4,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    4,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    4,    6, 0x80, 0x80),
			_mm256_setr_epi32(    5,    6, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    5,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    5,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    5,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    5,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    5,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    5,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    5,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    3,    5,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    5,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    5,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    5,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3,    5,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    5,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    5,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    5,    6, 0x80, 0x80),
			_mm256_setr_epi32(    4,    5,    6, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    4,    5,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    4,    5,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    4,    5,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    4,    5,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    4,    5,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    4,    5,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    4,    5,    6, 0x80, 0x80),
			_mm256_setr_epi32(    3,    4,    5,    6, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    4,    5,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    4,    5,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    4,    5,    6, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3,    4,    5,    6, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    4,    5,    6, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    4,    5,    6, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    4,    5,    6, 0x80),
			_mm256_setr_epi32(    7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    3,    7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    4,    7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    4,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    4,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    4,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    4,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    4,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    4,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    4,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    3,    4,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    4,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    4,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    4,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3,    4,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    4,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    4,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    4,    7, 0x80, 0x80),
			_mm256_setr_epi32(    5,    7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    5,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    5,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    5,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    5,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    5,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    5,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    5,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    3,    5,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    5,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    5,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    5,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3,    5,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    5,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    5,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    5,    7, 0x80, 0x80),
			_mm256_setr_epi32(    4,    5,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    4,    5,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    4,    5,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    4,    5,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    4,    5,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    4,    5,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    4,    5,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    4,    5,    7, 0x80, 0x80),
			_mm256_setr_epi32(    3,    4,    5,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    4,    5,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    4,    5,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    4,    5,    7, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3,    4,    5,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    4,    5,    7, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    4,    5,    7, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    4,    5,    7, 0x80),
			_mm256_setr_epi32(    6,    7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    6,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    6,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    6,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    3,    6,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    4,    6,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    4,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    4,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    4,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    4,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    4,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    4,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    4,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    3,    4,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    4,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    4,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    4,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3,    4,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    4,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    4,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    4,    6,    7, 0x80),
			_mm256_setr_epi32(    5,    6,    7, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    5,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    5,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    5,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    2,    5,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    5,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    5,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    5,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    3,    5,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    5,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    5,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    5,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    2,    3,    5,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    5,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    5,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    5,    6,    7, 0x80),
			_mm256_setr_epi32(    4,    5,    6,    7, 0x80, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    4,    5,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    1,    4,    5,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    4,    5,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    2,    4,    5,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    4,    5,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    1,    2,    4,    5,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    4,    5,    6,    7, 0x80),
			_mm256_setr_epi32(    3,    4,    5,    6,    7, 0x80, 0x80, 0x80),
			_mm256_setr_epi32(    0,    3,    4,    5,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    1,    3,    4,    5,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    0,    1,    3,    4,    5,    6,    7, 0x80),
			_mm256_setr_epi32(    2,    3,    4,    5,    6,    7, 0x80, 0x80),
			_mm256_setr_epi32(    0,    2,    3,    4,    5,    6,    7, 0x80),
			_mm256_setr_epi32(    1,    2,    3,    4,    5,    6,    7, 0x80),
			_mm256_setr_epi32(    0,    1,    2,    3,    4,    5,    6,    7)
		};

		// While queue is not empty
		while (qStart < qEnd) {
			uint32_t currElement = queue[qStart];
			qStart++;

			const uint32_t startEdge = off[currElement];

			const uint32_t stopEdge = off[currElement + 1];
			const uint32_t nextLevel = level[currElement] + 1;
			const __m256i mmNextLevel = _mm256_set1_epi32(nextLevel);
			uint32_t j;
			
			j = startEdge;
			for (; j + 8 < stopEdge; j += 8) {
				const uint32_t k0 = ind[j];
				const uint32_t k1 = ind[j + 1];
				const uint32_t k2 = ind[j + 2];
				const uint32_t k3 = ind[j + 3];
				const uint32_t k4 = ind[j + 4];
				const uint32_t k5 = ind[j + 5];
				const uint32_t k6 = ind[j + 6];
				const uint32_t k7 = ind[j + 7];
				const __m256i mmK = _mm256_loadu_si256((__m256i*)&ind[j]);
				__m256i mmLevelK = _mm256_inserti128_si256(
					_mm256_castsi128_si256(_mm_insert_epi32(_mm_insert_epi32(_mm_insert_epi32(_mm_cvtsi32_si128(level[k0]), level[k1], 1), level[k2], 2), level[k3], 3)),
					_mm_insert_epi32(_mm_insert_epi32(_mm_insert_epi32(_mm_cvtsi32_si128(level[k4]), level[k5], 1), level[k6], 2), level[k7], 3), 1);
				const __m256i predicate = _mm256_cmpgt_epi32(mmLevelK, mmNextLevel);
				const unsigned qMask = _mm256_movemask_ps(_mm256_castsi256_ps(predicate));
				_mm256_storeu_si256((__m256i*)&queue[qEnd],  _mm256_permutevar8x32_epi32(mmK, shuffleTable[qMask]));
				qEnd += __builtin_popcount(qMask);
				mmLevelK = _mm256_min_epi32(mmLevelK, mmNextLevel);
				const __m128i mmLevelKLo = _mm256_castsi256_si128(mmLevelK);
				const __m128i mmLevelKHi = _mm256_extracti128_si256(mmLevelK, 1);
				level[k0] = _mm_cvtsi128_si32(mmLevelKLo);
				level[k1] = _mm_extract_epi32(mmLevelKLo, 1);
				level[k2] = _mm_extract_epi32(mmLevelKLo, 2);
				level[k3] = _mm_extract_epi32(mmLevelKLo, 3);
				level[k4] = _mm_cvtsi128_si32(mmLevelKHi);
				level[k5] = _mm_extract_epi32(mmLevelKHi, 1);
				level[k6] = _mm_extract_epi32(mmLevelKHi, 2);
				level[k7] = _mm_extract_epi32(mmLevelKHi, 3);
			}
			for (; j < stopEdge; j ++) {
				const uint32_t k = ind[j];
				uint32_t levelK = level[k];
				queue[qEnd] = k;
				__asm__ __volatile__ (
					"CMPL %[levelK], %[nextLevel];"
					"CMOVNGEL %[nextLevel], %[levelK];"
					"ADCL $0, %[qEnd];"
					: [qEnd] "+r" (qEnd), [levelK] "+r" (levelK)
					: [nextLevel] "r" (nextLevel)
					: "cc"
				);
				level[k] = levelK;
			}

		}
		//~ printf("\nQE %d\n",qEnd);
	}
#endif

#ifdef __MIC__
	void BFS_TopDown_Branchless_MIC(uint32_t* off, uint32_t* ind, uint32_t* queue, uint32_t* level, uint32_t currRoot) {
		level[currRoot] = 0;

		queue[0] = currRoot;
		uint32_t qStart = 0, qEnd = 1;

		// While queue is not empty
		while (qStart < qEnd) {
			uint32_t currElement = queue[qStart];
			qStart++;

			const uint32_t startEdge = off[currElement];

			const uint32_t stopEdge = off[currElement + 1];
			const uint32_t nextLevel = level[currElement] + 1;
			const __m512i mmNextLevel = _mm512_set1_epi32(nextLevel);
			
			uint32_t j = startEdge;
			for (; j + 16 < stopEdge; j += 16) {
				__m512i k = _mm512_undefined_epi32();
				k = _mm512_loadunpacklo_epi32(k, &ind[j]);
				k = _mm512_loadunpackhi_epi32(k, &ind[j + 16]);
				__m512i levelK = _mm512_i32gather_epi32(k, level, sizeof(uint32_t));
				const __mmask16 predicate = _mm512_cmp_epi32_mask(levelK, mmNextLevel, _MM_CMPINT_GT);
				_mm512_mask_packstorelo_epi32(&queue[qEnd], predicate, k);
				_mm512_mask_packstorehi_epi32(&queue[qEnd + 16], predicate, k);
				qEnd += _mm_countbits_32(_mm512_mask2int(predicate));
				levelK = _mm512_min_epi32(levelK, mmNextLevel);
				_mm512_i32scatter_epi32(level, k, levelK, sizeof(uint32_t));
			}
			if (j != stopEdge) {
				const __mmask16 elements_mask = _mm512_int2mask((1 << (stopEdge - j)) - 1);
				__m512i k = _mm512_undefined_epi32();
				k = _mm512_mask_loadunpacklo_epi32(k, elements_mask, &ind[j]);
				k = _mm512_mask_loadunpackhi_epi32(k, elements_mask, &ind[j + 16]);
				__m512i levelK = _mm512_mask_i32gather_epi32(_mm512_undefined_epi32(), elements_mask, k, level, sizeof(uint32_t));
				const __mmask16 predicate = _mm512_mask_cmp_epi32_mask(elements_mask, levelK, mmNextLevel, _MM_CMPINT_GT);
				_mm512_mask_packstorelo_epi32(&queue[qEnd], predicate, k);
				_mm512_mask_packstorehi_epi32(&queue[qEnd + 16], predicate, k);
				qEnd += _mm_countbits_32(_mm512_mask2int(predicate));
				levelK = _mm512_mask_min_epi32(_mm512_undefined_epi32(), elements_mask, levelK, mmNextLevel);
				_mm512_mask_i32scatter_epi32(level, elements_mask, k, levelK, sizeof(uint32_t));
			}
		}
		//~ printf("\nQE %d\n",qEnd);
	}
#endif
