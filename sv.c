#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#if defined(__x86_64__)
	#include <x86intrin.h>
#endif

#ifdef __SSE4_1__
	bool ConnectedComponents_SV_Branchless_SSE4_1(size_t vertexCount, uint32_t* componentMap, uint32_t* vertexEdges, uint32_t* neighbors) {
		uint32_t changed = 0;

		for (size_t vertex=0; vertex < vertexCount; vertex++) {
			const uint32_t *restrict neighborPointer = &neighbors[vertexEdges[vertex]];
			const size_t vdeg = vertexEdges[vertex + 1] - vertexEdges[vertex];
			const uint32_t currentComponent = componentMap[vertex];
			uint32_t component_v_new = currentComponent;
			size_t edge = 0;
			if (vdeg >= 4) {
				__m128i vec_component_v_new = _mm_set1_epi32(component_v_new);
				for (; edge < (vdeg & -4); edge += 4){
					const uint32_t u0 = neighborPointer[edge];
					const uint32_t u1 = neighborPointer[edge + 1];
					const uint32_t u2 = neighborPointer[edge + 2];
					const uint32_t u3 = neighborPointer[edge + 3];
					const __m128i vec_component_u = _mm_insert_epi32(_mm_insert_epi32(_mm_insert_epi32(_mm_cvtsi32_si128(componentMap[u0]), componentMap[u1], 1), componentMap[u2], 2), componentMap[u3], 3);
					vec_component_v_new = _mm_min_epu32(vec_component_u, vec_component_v_new);
				}
				vec_component_v_new = _mm_min_epu32(vec_component_v_new, _mm_shuffle_epi32(vec_component_v_new, _MM_SHUFFLE(3, 2, 3, 2)));
				vec_component_v_new = _mm_min_epu32(vec_component_v_new, _mm_shuffle_epi32(vec_component_v_new, _MM_SHUFFLE(3, 2, 1, 1)));
				component_v_new = _mm_cvtsi128_si32(vec_component_v_new);
			}
			for (; edge < vdeg; edge += 1){
				const uint32_t neighborVertex = neighborPointer[edge];
				const uint32_t component_u = componentMap[neighborVertex];
				if (component_u < component_v_new) {
					component_v_new = component_u;
				}
			}
			changed |= currentComponent ^ component_v_new;
			componentMap[vertex] = component_v_new;
		}

		if (!changed)
			return false;

		//~ for (size_t i = 0; i < vertexCount; i++) {
			//~ while (componentMap[i] != componentMap[componentMap[i]]) {
				//~ componentMap[i] = componentMap[componentMap[i]];
			//~ }
		//~ }

		return true;
	}
#endif

#ifdef __MIC__
	bool ConnectedComponents_SV_Branchless_MIC(size_t vertexCount, uint32_t* componentMap, uint32_t* vertexEdges, uint32_t* neighbors) {
		__mmask16 changed = 0;

		for (size_t vertex=0; vertex < vertexCount; vertex++) {
			const uint32_t *restrict neighborPointer = &neighbors[vertexEdges[vertex]];
			const size_t vdeg = vertexEdges[vertex + 1] - vertexEdges[vertex];
			if (vdeg != 0) {
				const __m512i currentComponent = _mm512_extload_epi32(&componentMap[vertex], _MM_UPCONV_EPI32_NONE, _MM_BROADCAST_1X16, _MM_HINT_NONE);
				__m512i component_v_new = currentComponent;
				if (vdeg >= 16) {
					size_t edge = 0;
					for (; edge + 15 < vdeg; edge += 16){
						const __m512i neighborVertex = _mm512_loadunpackhi_epi32(_mm512_loadunpacklo_epi32(_mm512_undefined_epi32(), &neighborPointer[edge]), &neighborPointer[edge + 16]);
						const __m512i component_u = _mm512_i32gather_epi32(neighborVertex, componentMap, sizeof(uint32_t));
						component_v_new = _mm512_min_epu32(component_v_new, component_u);
					}
					const size_t remainder = vdeg - edge;
					if (remainder != 0) {
						__mmask16 mask = _mm512_int2mask((1 << remainder) - 1);
						const __m512i neighborVertex = _mm512_mask_loadunpackhi_epi32(_mm512_mask_loadunpacklo_epi32(_mm512_undefined_epi32(), mask, &neighborPointer[edge]), mask, &neighborPointer[edge + 16]);
						const __m512i component_u = _mm512_mask_i32gather_epi32(_mm512_undefined_epi32(), mask, neighborVertex, componentMap, sizeof(uint32_t));
						component_v_new = _mm512_mask_min_epu32(component_v_new, mask, component_v_new, component_u);
					}
					changed = _mm512_kor(changed, _mm512_cmp_epu32_mask(currentComponent, component_v_new, _MM_CMPINT_NE));
					componentMap[vertex] = _mm512_reduce_min_epu32(component_v_new);
				} else {
					__mmask16 mask = _mm512_int2mask((1 << vdeg) - 1);
					const __m512i neighborVertex = _mm512_mask_loadunpackhi_epi32(_mm512_mask_loadunpacklo_epi32(_mm512_undefined_epi32(), mask, &neighborPointer[0]), mask, &neighborPointer[16]);
					const __m512i component_u = _mm512_mask_i32gather_epi32(_mm512_undefined_epi32(), mask, neighborVertex, componentMap, sizeof(uint32_t));
					component_v_new = _mm512_mask_min_epu32(component_v_new, mask, component_v_new, component_u);
					changed = _mm512_kor(changed, _mm512_cmp_epu32_mask(currentComponent, component_v_new, _MM_CMPINT_NE));
					componentMap[vertex] = _mm512_mask_reduce_min_epu32(mask, component_v_new);
				}
			}
		}

		if (_mm512_kortestz(changed, changed))
			return false;

		//~ for (size_t i = 0; i < vertexCount; i++) {
			//~ while (componentMap[i] != componentMap[componentMap[i]]) {
				//~ componentMap[i] = componentMap[componentMap[i]];
			//~ }
		//~ }

		return true;
	}
#endif
