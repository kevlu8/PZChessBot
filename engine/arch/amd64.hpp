#pragma once

#include "arch.hpp"

#if defined(__amd64__)

namespace arch {
	uint64_t pext(uint64_t x, uint64_t y) {
		return _pext_u64(x, y);
	}
}

#endif
