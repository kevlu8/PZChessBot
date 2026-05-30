#pragma once

#include "arch.hpp"

namespace arch {
	uint64_t tzcnt(uint64_t x) {
		return __builtin_ctzll(x);
	}

	uint64_t lzcnt(uint64_t x) {
		return __builtin_clzll(x);
	}

	uint64_t popcnt(uint64_t x) {
		return __builtin_popcountll(x);
	}

	uint64_t blsi(uint64_t x) {
		return x & -x;
	}

	uint64_t blsr(uint64_t x) {
		return x & (x - 1);
	}

	uint64_t blsmsk(uint64_t x) {
		return x ^ (x - 1);
	}

	void prefetch(void *p) {
		__builtin_prefetch(p);
	}
}
