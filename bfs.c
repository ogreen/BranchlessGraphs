#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

void BFSSeq(uint32_t* off, uint32_t* ind, uint32_t* Queue, uint32_t* level, uint32_t currRoot) {
	level[currRoot] = 0;


	Queue[0] = currRoot;
	uint32_t qStart = 0, qEnd = 1;

	// While queue is not empty
	while (qStart!=qEnd) {
		uint64_t currElement = Queue[qStart];
		qStart++;

		uint32_t startEdge = off[currElement];
		uint32_t stopEdge = off[currElement+1];

		uint32_t nextLevel = level[currElement]+1;
		for (uint32_t j = startEdge; startEdge < stopEdge; startEdge++) {
			uint32_t k = ind[startEdge];

			// If this is a neighbor and has not been found
			if (level[k] > level[currElement]) {
				// Checking if "k" has been found.
				if (level[k] == INT32_MAX) {
					level[k] = nextLevel; //level[currElement]+1;
					Queue[qEnd++] = k;
				}
			}
		}

	}

	printf("\nQE %d\n",qEnd);
}

void BFSSeqBranchless(uint32_t* off, uint32_t* ind, uint32_t* Queue, uint32_t* level, uint32_t currRoot) {

	level[currRoot] = 0;

	Queue[0] = currRoot;
	uint32_t qStart=0,qEnd=1;

	uint32_t flag=1;
	uint32_t isINF=1;
	uint32_t pre=0;
	uint32_t temp;
	// While queue is not empty
	while (qStart < qEnd) {
		uint32_t currElement = Queue[qStart++];

		uint32_t startEdge = off[currElement];
		uint32_t stopEdge = off[currElement+1];
		uint32_t nextLevel=level[currElement]+1;
		for (uint32_t j = startEdge; j < stopEdge; j++) {
			const int64_t k = ind[j];
			temp = nextLevel - level[k];
			Queue[qEnd] = k;
			isINF = (uint32_t)(temp) >> 31;
			qEnd += isINF;
			level[k] += isINF * (temp);
		}
	}
	printf("\nQE %d\n",qEnd);
}

#ifndef __MIC__
	void BFSSeqBranchlessAsm(uint32_t* off, uint32_t* ind, uint32_t* Queue, uint32_t* level, uint32_t currRoot) {
		level[currRoot] = 0;

		Queue[0] = currRoot;
		uint32_t qStart = 0, qEnd = 1;

		// While queue is not empty
		while (qStart < qEnd) {
			uint32_t currElement = Queue[qStart++];

			const uint32_t startEdge = off[currElement];
			const uint32_t stopEdge = off[currElement + 1];
			const uint32_t nextLevel = level[currElement] + 1;
			for (uint32_t j = startEdge; j < stopEdge; j++) {
				const uint32_t k = ind[j];
				uint32_t levelK = level[k];
				Queue[qEnd] = k;
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
		printf("\nQE %d\n",qEnd);
	}
#endif

#include <x86intrin.h>

/*
void BFSSeqBranchlessSSE(int64_t* off, int64_t* ind, int64_t* Queue, int64_t* level, int64_t currRoot) {
	level[currRoot] = 0;

	Queue[0] = currRoot;
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
		currElement = Queue[qStart];
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
			_mm_storeu_si128((__m128i*)&Queue[qEnd], _mm_shuffle_epi8(mmK, shuffleTable[qMask]));
			qEnd += __builtin_popcount(qMask);
			mmLevelK = _mm_blendv_epi8(mmLevelK, mmNextLevel, predicate);
			level[k0] = _mm_cvtsi128_si64(mmLevelK);
			level[k1] = _mm_extract_epi64(mmLevelK, 1);
		}
		for (; j < stopEdge; j ++) {
			const int64_t k = ind[j];
			int64_t levelK = level[k];
			Queue[qEnd] = k;
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
	printf("\nQE %d\n",qEnd);
}
*/

#ifdef __SSE__
	void BFSSeqBranchlessSSE(uint32_t* off, uint32_t* ind, uint32_t* Queue, uint32_t* level, uint32_t currRoot) {
		level[currRoot] = 0;

		Queue[0] = currRoot;
		uint32_t qStart = 0, qEnd = 1;

		const __m128i shuffleTable[16] = {
			_mm_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm_setr_epi8(   0,    1,    2,    3, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm_setr_epi8(   4,    5,    6,    7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm_setr_epi8(   0,    1,    2,    3,    4,    5,    6,    7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm_setr_epi8(   8,    9,   10,   11, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm_setr_epi8(   0,    1,    2,    3,    8,    9,   10,   11, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm_setr_epi8(   4,    5,    6,    7,    8,    9,   10,   11, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm_setr_epi8(   0,    1,    2,    3,    4,    5,    6,    7,    8,    9,   10,   11, 0x80, 0x80, 0x80, 0x80),
			_mm_setr_epi8(  12,   13,   14,   15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm_setr_epi8(   0,    1,    2,    3,   12,   13,   14,   15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm_setr_epi8(   4,    5,    6,    7,   12,   13,   14,   15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm_setr_epi8(   0,    1,    2,    3,    4,    5,    6,    7,   12,   13,   14,   15, 0x80, 0x80, 0x80, 0x80),
			_mm_setr_epi8(   8,    9,   10,   11,   12,   13,   14,   15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80),
			_mm_setr_epi8(   0,    1,    2,    3,    8,    9,   10,   11,   12,   13,   14,   15, 0x80, 0x80, 0x80, 0x80),
			_mm_setr_epi8(   4,    5,    6,    7,    8,    9,   10,   11,   12,   13,   14,   15, 0x80, 0x80, 0x80, 0x80),
			_mm_setr_epi8(   0,    1,    2,    3,    4,    5,    6,    7,    8,    9,   10,   11,   12,   13,   14,   15)
		};

		// While queue is not empty
		while (qStart < qEnd) {
			uint32_t currElement = Queue[qStart];
			qStart++;

			const uint32_t startEdge = off[currElement];

			const uint32_t stopEdge = off[currElement + 1];
			const uint32_t nextLevel = level[currElement] + 1;
			const __m128i mmNextLevel = _mm_set1_epi32(nextLevel);
			
			uint32_t j = startEdge;
			for (; j + 4 < stopEdge; j += 4) {
				const uint32_t k0 = ind[j];
				const uint32_t k1 = ind[j + 1];
				const uint32_t k2 = ind[j + 2];
				const uint32_t k3 = ind[j + 3];
				const __m128i mmK = _mm_loadu_si128((__m128i*)&ind[j]);
				__m128i mmLevelK = _mm_insert_epi32(_mm_insert_epi32(_mm_insert_epi32(_mm_cvtsi32_si128(level[k0]), level[k1], 1), level[k2], 2), level[k3], 3);
				__m128i predicate = _mm_cmpgt_epi32(mmLevelK, mmNextLevel);
				const unsigned qMask = _mm_movemask_ps(_mm_castsi128_ps(predicate));
				//const unsigned qMask = _mm_movemask_epi8(predicate);

				//predicate = _mm_and_si128(predicate, _mm_set1_epi8(4));
				//__m128i mask = predicate;
				/*
				mask = _mm_add_epi8(mask, _mm_srli_si128(predicate, 4));
				mask = _mm_add_epi8(mask, _mm_srli_si128(predicate, 8));
				mask = _mm_add_epi8(mask, _mm_srli_si128(predicate, 12));
				*/
				//~ mask = _mm_add_epi8(mask, _mm_srli_si128(mask, 4));
				//~ mask = _mm_add_epi8(mask, _mm_srli_si128(mask, 8));
				//~ mask = _mm_add_epi8(mask, _mm_setr_epi8(0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3));
				//const __m128i compressed = _mm_shuffle_epi8(mmK, mask);
				const __m128i compressed = _mm_shuffle_epi8(mmK, shuffleTable[qMask]);
				_mm_storeu_si128((__m128i*)&Queue[qEnd], compressed);

				qEnd += __builtin_popcount(qMask);
				//~ qEnd += __builtin_popcount(qMask) >> 2;
				mmLevelK = _mm_min_epi32(mmLevelK, mmNextLevel);
				level[k0] = _mm_cvtsi128_si32(mmLevelK);
				level[k1] = _mm_extract_epi32(mmLevelK, 1);
				level[k2] = _mm_extract_epi32(mmLevelK, 2);
				level[k3] = _mm_extract_epi32(mmLevelK, 3);
			}
			for (; j < stopEdge; j ++) {
				const uint32_t k = ind[j];
				uint32_t levelK = level[k];
				Queue[qEnd] = k;
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
		printf("\nQE %d\n",qEnd);
	}
#endif

#ifdef __AVX2__
	void BFSSeqBranchlessAVX2(uint32_t* off, uint32_t* ind, uint32_t* Queue, uint32_t* level, uint32_t currRoot) {
		level[currRoot] = 0;

		Queue[0] = currRoot;
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
			uint32_t currElement = Queue[qStart];
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
				_mm256_storeu_si256((__m256i*)&Queue[qEnd],  _mm256_permutevar8x32_epi32(mmK, shuffleTable[qMask]));
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
				Queue[qEnd] = k;
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
		printf("\nQE %d\n",qEnd);
	}
#endif

#ifdef __MIC__
	void BFSSeqBranchlessMICPartVec(uint32_t* off, uint32_t* ind, uint32_t* Queue, uint32_t* level, uint32_t currRoot) {
		level[currRoot] = 0;

		Queue[0] = currRoot;
		uint32_t qStart = 0, qEnd = 1;

		// While queue is not empty
		while (qStart < qEnd) {
			uint32_t currElement = Queue[qStart];
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
				_mm512_mask_packstorelo_epi32(&Queue[qEnd], predicate, k);
				_mm512_mask_packstorehi_epi32(&Queue[qEnd + 16], predicate, k);
				qEnd += _mm_countbits_32(_mm512_mask2int(predicate));
				levelK = _mm512_min_epi32(levelK, mmNextLevel);
				_mm512_i32scatter_epi32(level, k, levelK, sizeof(uint32_t));
			}
			for (; j < stopEdge; j ++) {
				const uint32_t k = ind[j];
				uint32_t levelK = level[k];

				// If this is a neighbor and has not been found
				if (levelK > level[currElement]) {
					// Checking if "k" has been found.
					if (levelK == INT32_MAX) {
						level[k] = nextLevel; //level[currElement]+1;
						Queue[qEnd++] = k;
					}
				}
			}
		}
		printf("\nQE %d\n",qEnd);
	}

	void BFSSeqBranchlessMICFullVec(uint32_t* off, uint32_t* ind, uint32_t* Queue, uint32_t* level, uint32_t currRoot) {
		level[currRoot] = 0;

		Queue[0] = currRoot;
		uint32_t qStart = 0, qEnd = 1;

		// While queue is not empty
		while (qStart < qEnd) {
			uint32_t currElement = Queue[qStart];
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
				_mm512_mask_packstorelo_epi32(&Queue[qEnd], predicate, k);
				_mm512_mask_packstorehi_epi32(&Queue[qEnd + 16], predicate, k);
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
				_mm512_mask_packstorelo_epi32(&Queue[qEnd], predicate, k);
				_mm512_mask_packstorehi_epi32(&Queue[qEnd + 16], predicate, k);
				qEnd += _mm_countbits_32(_mm512_mask2int(predicate));
				levelK = _mm512_mask_min_epi32(_mm512_undefined_epi32(), elements_mask, levelK, mmNextLevel);
				_mm512_mask_i32scatter_epi32(level, elements_mask, k, levelK, sizeof(uint32_t));
			}
		}
		printf("\nQE %d\n",qEnd);
	}
#endif
