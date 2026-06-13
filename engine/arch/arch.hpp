/*
 * PZChessBot, a UCI chess engine
 * Copyright (C) 2026 Kevin Lu and William Ma
 *
 * PZChessBot is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PZChessBot is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with PZChessBot. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstdint>

// Define constants for each target
#if defined(__ARM_NEON)
#define TARGET_ARM_NEON
#elif defined(__AVX512BW__)
#define TARGET_X86_AVX512
#elif defined(__AVX2__)
#define TARGET_X86_AVX2
#else
#error Unsupported architecture
#endif

#include "simd/simd.hpp"

// Determine pext usage
#if defined(__BMI2__) && !defined(__znver1__) && !defined(__znver2__)
#define USE_PEXT
#endif

namespace arch {
	// Count the Number of Trailing Zero Bits
	uint64_t tzcnt(uint64_t x);
	// Count the Number of Leading Zero Bits
	/// NOTE: `x` must never be 0
	uint64_t lzcnt(uint64_t x);

	// Return the Count of Number of Bits Set to 1
	uint64_t popcnt(uint64_t x);

	// Extract Lowest Set Isolated Bit
	uint64_t blsi(uint64_t x);
	// Reset Lowest Set Bit
	uint64_t blsr(uint64_t x);
	// Get Mask Up to Lowest Set Bit
	uint64_t blsmsk(uint64_t x);

#if defined(USE_PEXT)
	// Parallel extract of bits from `x` using mask in `y`
	uint64_t pext(uint64_t x, uint64_t y);
#endif

	// Move data closer to the processor using T0 hint
	void prefetch(void *p);
}
