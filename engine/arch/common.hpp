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
