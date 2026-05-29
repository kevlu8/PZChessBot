#pragma once

#include "../arch.hpp"

#if defined(__amd64__)

namespace arch {
	uint64_t tzcnt(uint64_t x) {
		return __tzcnt_u64(x);
	}

	uint64_t lzcnt(uint64_t x) {
		// x is always nonzero
		return __builtin_clzll(x);
	}

	uint64_t popcnt(uint64_t x) {
		return _mm_popcnt_u64(x);
	}

	uint64_t blsi(uint64_t x) {
		return __blsi_u64(x);
	}

	uint64_t blsr(uint64_t x) {
		return __blsr_u64(x);
	}

	uint64_t blsmsk(uint64_t x) {
		return __blsmsk_u64(x);
	}

#if defined(USE_PEXT)
	uint64_t pext(uint64_t x, uint64_t y) {
		return _pext_u64(x, y);
	}
#endif

	void prefetcht0(void *p) {
		return _mm_prefetch(p, _MM_HINT_T0);
	}
}

#endif
