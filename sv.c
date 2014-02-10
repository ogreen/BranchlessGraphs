#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#if defined(__x86_64__)
	#include <x86intrin.h>
#endif

#define SV_USE_INDEX

bool SVSeq(size_t nv, uint32_t* component_map, uint32_t* off, uint32_t* ind) {
	bool changed = 0;

#if defined(SV_USE_INDEX)
	uint32_t *restrict vindend = ind + off[0];
	for (size_t v = 0; v < nv; v++) {
		const uint32_t *restrict vind = vindend;
		vindend = ind + off[v+1];
		const uint32_t component_v = component_map[v];
		uint32_t component_v_new = component_v;
		for (; vind != vindend; vind++){
			const uint32_t u = *vind;
			const uint32_t component_u = component_map[u];
			if (component_u < component_v_new) {
				component_map[v] = component_u;
				changed = true;
			}
		}
	}
#else
	uint32_t* vindend = ind + (*off++);
	uint32_t* component_map_v = component_map;
	for (size_t v = nv; v != 0; v--) {
		const uint32_t *restrict vind = vindend;
		vindend = ind + (*off++);
		const uint32_t component_v = *component_map_v;
		uint32_t component_v_new = component_v;
		for (; vind != vindend; vind++){
			const uint32_t u = *vind;
			const uint32_t component_u = component_map[u];
			if (component_u < component_v_new) {
				*component_map_v = component_u;
				changed = true;
			}
		}
		component_map_v++;
	}
#endif

	if (!changed)
		return false;

	//~ for (size_t i = 0; i < nv; i++) {
		//~ while (component_map[i] != component_map[component_map[i]]) {
			//~ component_map[i] = component_map[component_map[i]];
		//~ }
	//~ }

	return true;
}

bool SVBranchless(size_t nv, uint32_t* component_map, uint32_t* off, uint32_t* ind) {
	uint32_t changed = 0;

#if defined(SV_USE_INDEX)
	uint32_t *restrict vindend = ind + off[0];
	for (size_t v = 0; v < nv; v++) {
		const uint32_t *restrict vind = vindend;
		vindend = ind + off[v+1];
		const uint32_t component_v = component_map[v];
		uint32_t component_v_new = component_v;
		for (; vind != vindend; vind++){
			const uint32_t u = *vind;
			const uint32_t component_u = component_map[u];
			const uint32_t component_diff = component_u - component_v_new;
			const uint32_t flag = (uint32_t)((int32_t)component_u - (int32_t)component_v_new) >> 31;
			component_v_new += (-flag) & (component_u - component_v_new);
		}
		changed |= component_v ^ component_v_new;
		component_map[v] = component_v_new;
	}
#else
	uint32_t* vindend = ind + (*off++);
	uint32_t* component_map_v = component_map;
	for (size_t v = nv; v != 0; v--) {
		const uint32_t *restrict vind = vindend;
		vindend = ind + (*off++);
		const uint32_t component_v = *component_map_v;
		uint32_t component_v_new = component_v;
		for (; vind != vindend; vind++){
			const uint32_t u = *vind;
			const uint32_t component_u = component_map[u];
			const uint32_t component_diff = component_u - component_v_new;
			const uint32_t flag = (uint32_t)((int32_t)component_u - (int32_t)component_v_new) >> 31;
			component_v_new += (-flag) & (component_u - component_v_new);
		}
		changed |= component_v ^ component_v_new;
		*component_map_v++ = component_v_new;
	}
#endif

	if (!changed)
		return false;

	//~ for (size_t i = 0; i < nv; i++) {
		//~ while (component_map[i] != component_map[component_map[i]]) {
			//~ component_map[i] = component_map[component_map[i]];
		//~ }
	//~ }

	return true;
}

#ifndef __MIC__
	bool SVBranchlessAsm(size_t nv, uint32_t* component_map, uint32_t* off, uint32_t* ind) {
		uint32_t changed = 0;

		#if defined(SV_USE_INDEX)
			uint32_t *restrict vindend = ind + off[0];
			for (size_t v = 0; v < nv; v++) {
				const uint32_t *restrict vind = vindend;
				vindend = ind + off[v+1];
				const uint32_t component_v = component_map[v];
				uint32_t component_v_new = component_v;
				for (; vind != vindend; vind++){
					const uint32_t u = *vind;
					const uint32_t component_u = component_map[u];
					__asm__ __volatile__ (
						"CMPL %[component_v_new], %[component_u];"
						"CMOVBL %[component_u], %[component_v_new];"
						: [component_v_new] "+r" (component_v_new)
						: [component_u] "r" (component_u)
						: "cc"
					);
				}
				changed |= component_v ^ component_v_new;
				component_map[v] = component_v_new;
			}
		#else
			uint32_t* vindend = ind + (*off++);
			uint32_t* component_map_v = component_map;
			for (size_t v = nv; v != 0; v--) {
				const uint32_t *restrict vind = vindend;
				vindend = ind + (*off++);
				const uint32_t component_v = *component_map_v;
				uint32_t component_v_new = component_v;
				for (; vind != vindend; vind++){
					const uint32_t u = *vind;
					const uint32_t component_u = component_map[u];
					__asm__ __volatile__ (
						"CMPL %[component_v_new], %[component_u];"
						"CMOVBL %[component_u], %[component_v_new];"
						: [component_v_new] "+r" (component_v_new)
						: [component_u] "r" (component_u)
						: "cc"
					);
				}
				changed |= component_v ^ component_v_new;
				*component_map_v++ = component_v_new;
			}
		#endif

		if (!changed)
			return false;

		//~ for (size_t i = 0; i < nv; i++) {
			//~ while (component_map[i] != component_map[component_map[i]]) {
				//~ component_map[i] = component_map[component_map[i]];
			//~ }
		//~ }

		return true;
	}
#endif

#ifdef __SSE4_1__
	bool SVBranchlessSSE4_1(size_t nv, uint32_t* component_map, uint32_t* off, uint32_t* ind) {
		uint32_t changed = 0;

		for (size_t v=0; v < nv; v++) {
			const uint32_t *restrict vind = &ind[off[v]];
			const size_t vdeg = off[v + 1] - off[v];
			const uint32_t component_v = component_map[v];
			uint32_t component_v_new = component_v;
			size_t edge = 0;
			if (vdeg >= 4) {
				__m128i vec_component_v_new = _mm_set1_epi32(component_v_new);
				for (; edge < (vdeg & -4); edge += 4){
					const uint32_t u0 = vind[edge];
					const uint32_t u1 = vind[edge + 1];
					const uint32_t u2 = vind[edge + 2];
					const uint32_t u3 = vind[edge + 3];
					const __m128i vec_component_u = _mm_insert_epi32(_mm_insert_epi32(_mm_insert_epi32(_mm_cvtsi32_si128(component_map[u0]), component_map[u1], 1), component_map[u2], 2), component_map[u3], 3);
					vec_component_v_new = _mm_min_epu32(vec_component_u, vec_component_v_new);
				}
				vec_component_v_new = _mm_min_epu32(vec_component_v_new, _mm_shuffle_epi32(vec_component_v_new, _MM_SHUFFLE(3, 2, 3, 2)));
				vec_component_v_new = _mm_min_epu32(vec_component_v_new, _mm_shuffle_epi32(vec_component_v_new, _MM_SHUFFLE(3, 2, 1, 1)));
				component_v_new = _mm_cvtsi128_si32(vec_component_v_new);
			}
			for (; edge < vdeg; edge += 1){
				const uint32_t u = vind[edge];
				const uint32_t component_u = component_map[u];
				if (component_u < component_v_new) {
					component_v_new = component_u;
				}
			}
			changed |= component_v ^ component_v_new;
			component_map[v] = component_v_new;
		}

		if (!changed)
			return false;

		//~ for (size_t i = 0; i < nv; i++) {
			//~ while (component_map[i] != component_map[component_map[i]]) {
				//~ component_map[i] = component_map[component_map[i]];
			//~ }
		//~ }

		return true;
	}
#endif

#ifdef __MIC__
	bool SVBranchlessMIC(size_t nv, uint32_t* component_map, uint32_t* off, uint32_t* ind) {
		__mmask16 changed = 0;

		for (size_t v=0; v < nv; v++) {
			const uint32_t *restrict vind = &ind[off[v]];
			const size_t vdeg = off[v + 1] - off[v];
			if (vdeg != 0) {
				const __m512i component_v = _mm512_extload_epi32(&component_map[v], _MM_UPCONV_EPI32_NONE, _MM_BROADCAST_1X16, _MM_HINT_NONE);
				__m512i component_v_new = component_v;
				if (vdeg >= 16) {
					size_t edge = 0;
					for (; edge + 15 < vdeg; edge += 16){
						const __m512i u = _mm512_loadunpackhi_epi32(_mm512_loadunpacklo_epi32(_mm512_undefined_epi32(), &vind[edge]), &vind[edge + 16]);
						const __m512i component_u = _mm512_i32gather_epi32(u, component_map, sizeof(uint32_t));
						component_v_new = _mm512_min_epu32(component_v_new, component_u);
					}
					const size_t remainder = vdeg - edge;
					if (remainder != 0) {
						__mmask16 mask = _mm512_int2mask((1 << remainder) - 1);
						const __m512i u = _mm512_mask_loadunpackhi_epi32(_mm512_mask_loadunpacklo_epi32(_mm512_undefined_epi32(), mask, &vind[edge]), mask, &vind[edge + 16]);
						const __m512i component_u = _mm512_mask_i32gather_epi32(_mm512_undefined_epi32(), mask, u, component_map, sizeof(uint32_t));
						component_v_new = _mm512_mask_min_epu32(component_v_new, mask, component_v_new, component_u);
					}
					changed = _mm512_kor(changed, _mm512_cmp_epu32_mask(component_v, component_v_new, _MM_CMPINT_NE));
					component_map[v] = _mm512_reduce_min_epu32(component_v_new);
				} else {
					__mmask16 mask = _mm512_int2mask((1 << vdeg) - 1);
					const __m512i u = _mm512_mask_loadunpackhi_epi32(_mm512_mask_loadunpacklo_epi32(_mm512_undefined_epi32(), mask, &vind[0]), mask, &vind[16]);
					const __m512i component_u = _mm512_mask_i32gather_epi32(_mm512_undefined_epi32(), mask, u, component_map, sizeof(uint32_t));
					component_v_new = _mm512_mask_min_epu32(component_v_new, mask, component_v_new, component_u);
					changed = _mm512_kor(changed, _mm512_cmp_epu32_mask(component_v, component_v_new, _MM_CMPINT_NE));
					component_map[v] = _mm512_mask_reduce_min_epu32(mask, component_v_new);
				}
			}
		}

		if (_mm512_kortestz(changed, changed))
			return false;

		//~ for (size_t i = 0; i < nv; i++) {
			//~ while (component_map[i] != component_map[component_map[i]]) {
				//~ component_map[i] = component_map[component_map[i]];
			//~ }
		//~ }

		return true;
	}
#endif
