#pragma once

#include "arch.hpp"

#if defined(__aarch64__)

namespace arch {
	// For future addition if needed

	// Temp
	uint64_t pext(uint64_t x, uint64_t y) {
		uint64_t result = 0;
		uint64_t dst_bit = 0;

		while (y) {
			uint64_t lowest = blsi(y); // isolate lowest set bit in mask
			uint64_t src_bit = tzcnt(lowest); // its position

			result |= ((x >> src_bit) & 1) << dst_bit;

			dst_bit++;
			y = blsr(y); // clear lowest set bit
		}
		return result;
	}
}

#endif
