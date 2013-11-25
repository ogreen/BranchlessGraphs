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
#include <xmmintrin.h>

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
