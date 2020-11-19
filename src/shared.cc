#include "pch.h"
#include "shared.h"

int p_compare(const void* in_a, const void* in_b) {
	const path_t* a = (const path_t*)in_a;
	const path_t* b = (const path_t*)in_b;
	return strncmp(a->s, b->s, path_n);
}
