#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#ifdef __x86_64__
	#include <x86intrin.h>
#endif
#include "timer.h"

#define BFS_USE_BITSETS 1
#define BFS_USE_ASM_BITOPS 0

inline void SetBit(uint32_t* bitset, size_t bit) {
#if BFS_USE_ASM_BITOPS
	if (__builtin_constant_p(bit)) {
		*bitset |= 1u << bit;
	} else {
		__asm__ __volatile__ (
			"BTS %[bit] ,%[bitset];"
			: [bitset] "+m" (*bitset)
			: [bit] "r" (bit)
			: "cc"
		);
	}
#else
	*bitset |= 1u << bit;
#endif
}

inline uint32_t IsSet(const uint32_t* bitset, size_t bit) {
#if BFS_USE_ASM_BITOPS
	if (__builtin_constant_p(bit)) {
		const uint32_t bitmask = 1u << bit;
		return (*bitset) & bitmask;
	} else {
		register uint8_t result;
		__asm__ __volatile__ (
			"BT %[bit] ,%[bitset];"
			"SETC %[result];"
			: [result] "=r" (result)
			: [bitset] "m" (*bitset), [bit] "r" (bit)
			: "cc"
		);
		return result;
	}
#else
	const uint32_t bitmask = 1 << bit;
	return (*bitset) & bitmask;
#endif
}

inline uint32_t IsReset(const uint32_t* bitset, size_t bit) {
#if BFS_USE_ASM_BITOPS
	if (__builtin_constant_p(bit)) {
		const uint32_t bitmask = 1u << bit;
		return ((*bitset) & bitmask) ^ bitmask;
	} else {
		register uint8_t result;
		__asm__ __volatile__ (
			"BT %[bit] ,%[bitset];"
			"SETNC %[result];"
			: [result] "=r" (result)
			: [bitset] "m" (*bitset), [bit] "r" (bit)
			: "cc"
		);
		return result;
	}
#else
	const uint32_t bitmask = 1u << bit;
	return ((*bitset) & bitmask) ^ bitmask;
#endif
}

uint32_t BFS_BottomUp_Branchy_LevelInformation(uint32_t* off, uint32_t* ind, uint32_t* bitmap, uint32_t* level, uint32_t currRoot, uint32_t nv, uint32_t currLevel, uint32_t* edgesTraversed, uint32_t* verticesFound) {
	const uint32_t nextLevel = currLevel + 1;
	uint32_t changed = 0;

	verticesFound[currLevel] = 0;
	edgesTraversed[currLevel] = 0;

	for (uint32_t v = 0; v != nv; v++) {
		if (!bitmap[v]) {
			uint32_t startEdge = off[v];
			uint32_t stopEdge = off[v+1];
			for (uint32_t j = startEdge; startEdge < stopEdge; startEdge++) {
				uint32_t k = ind[startEdge];

				// If this is a neighbor and has not been found
				if (level[k] == currLevel) {
					level[v] = nextLevel;
					bitmap[v] = 1;
					changed = 1;
					verticesFound[currLevel] += 1;
					edgesTraversed[currLevel] += j;
					break;
				}
			}
		}
	}
	return changed;
}

uint32_t BFS_BottomUp_Branchy(uint32_t* off, uint32_t* ind, uint32_t* bitmap, uint32_t* level, uint32_t currRoot, uint32_t nv, uint32_t currLevel) {
	const uint32_t nextLevel = currLevel + 1;
	uint32_t changed=0;

	uint32_t stopEdge = off[0];
	for (uint32_t v = 0; v != nv; v++) {
#if BFS_USE_BITSETS
		if (IsReset(bitmap + (v / 32), v % 32)) {
#else
		if (!bitmap[v / 32]) {
#endif
			const uint32_t startEdge = stopEdge;
			stopEdge = off[v+1];
			for (uint32_t edge = startEdge; edge < stopEdge; edge++) {
				uint32_t k = ind[edge];

				// If this is a neighbor and has not been found
				if (level[k] == currLevel) {
					level[v] = nextLevel;
#if BFS_USE_BITSETS
					SetBit(bitmap + (v / 32), v % 32);
#else
					bitmap[v] = 1;
#endif
					changed = 1;
					break;
				}
			}
		}
	}
	return changed;
}

uint32_t BFS_BottomUp_Branchless(uint32_t* off, uint32_t* ind, uint32_t* bitmap, uint32_t* level, uint32_t currRoot, uint32_t nv, uint32_t currLevel) {
	const uint32_t nextLevel = currLevel + 1;
	uint32_t changed=0;

	uint32_t stopEdge = off[0];
	for (uint32_t v = 0; v != nv; v++) {
		const uint32_t startEdge = stopEdge;
		stopEdge = off[v+1];
		const uint32_t levelV = level[v];
		uint32_t newLevelV = levelV;
		for (uint32_t edge = startEdge; edge != stopEdge; edge++) {
			uint32_t k = ind[edge];

			// If this is a neighbor and has not been found
			const uint32_t levelK = level[k];
			const uint32_t mask = -((int32_t)(levelK == currLevel));
			newLevelV ^= (newLevelV ^ nextLevel) & mask;
		}
		changed |= newLevelV ^ levelV;
	}
	return changed;
}

#if defined(__x86_64__) && !defined(__MIC__)
	uint32_t BFS_BottomUp_Branchless_CMOV(uint32_t* off, uint32_t* ind, uint32_t* bitmap, uint32_t* level, uint32_t currRoot, uint32_t nv, uint32_t currLevel) {
		const uint32_t nextLevel = currLevel + 1;
		uint32_t changed=0;

		uint32_t stopEdge = off[0];
		for (uint32_t v = 0; v != nv; v++) {
			const uint32_t startEdge = stopEdge;
			stopEdge = off[v+1];
			const uint32_t levelV = level[v];
			uint32_t newLevelV = levelV;
			for (uint32_t edge = startEdge; edge != stopEdge; edge++) {
				uint32_t k = ind[edge];

				// If this is a neighbor and has not been found
				const uint32_t levelK = level[k];
				__asm__ __volatile__ (
					"CMPL %[levelK], %[currLevel];"
					"CMOVE %[nextLevel], %[newLevelV];"
					: [newLevelV] "+r" (newLevelV)
					: [levelK] "r" (levelK), [currLevel] "r" (currLevel), [nextLevel] "r" (nextLevel)
					: "cc"
				);
			}
			changed |= newLevelV ^ levelV;
		}
		return changed;
	}
#endif
