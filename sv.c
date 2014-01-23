#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

bool SVSeq(size_t nv, uint32_t* component_map, uint32_t* off, uint32_t* ind) {
	bool changed = false;

	for (size_t v=0; v < nv; v++) {
		const uint32_t *restrict vind = &ind[off[v]];
		const size_t vdeg = off[v + 1] - off[v];

		for (size_t edge = 0; edge < vdeg; edge++){
			const uint32_t u = vind[edge];
			if (component_map[u] < component_map[v]) {
				component_map[v] = component_map[u];
				changed = true;
			}
		}
	}

	if (!changed)
		return false;

	for (size_t i = 0; i < nv; i++) {
		while (component_map[i] != component_map[component_map[i]]) {
			component_map[i] = component_map[component_map[i]];
		}
	}

	return true;
}

bool SVBranchless(size_t nv, uint32_t* component_map, uint32_t* off, uint32_t* ind) {
	uint32_t changed = false;

	for (size_t v=0; v < nv; v++) {
		const uint32_t *restrict vind = &ind[off[v]];
		const size_t vdeg = off[v + 1] - off[v];

		for (size_t edge = 0; edge < vdeg; edge++){
			const uint32_t u = vind[edge];
			const uint32_t flag = (uint32_t)((int32_t)component_map[u]-(int32_t)component_map[v]) >> 31;
			component_map[v] += (-flag) & (component_map[u] - component_map[v]);
			changed += flag;
		}
	}

	if (!changed)
		return false;

	for (size_t i = 0; i < nv; i++) {
		while (component_map[i] != component_map[component_map[i]]) {
			component_map[i] = component_map[component_map[i]];
		}
	}
	return true;
}
