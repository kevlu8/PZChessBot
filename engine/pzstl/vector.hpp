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

#include <cstddef>
#include <cstdint>

#define PZSTD_DEFAULT_SIZE 256

namespace pzstd {
	template <typename T, size_t MaxSize = PZSTD_DEFAULT_SIZE>
	struct vector {
		// It is necessary to write this class using static allocation only
		union { T data[MaxSize]; };
		uint16_t sz;

		vector() : sz(0) {}

		void push_back(const T &value) {
			new (&data[sz++]) T(value);
		}
		void push_back(T &&value) {
			new (&data[sz++]) T(value);
		}
		void pop_back() {
			~T(&data[--sz]);
		}
		void clear() {
			while (sz > 0) {
				~T(&data[--sz]);
			}
		}
		uint16_t count(const T &value) const {
			uint16_t cnt = 0;
			for (uint16_t i = 0; i < sz; i++) {
				if (data[i] == value) {
					cnt++;
				}
			}
			return cnt;
		}
		T &operator[](uint16_t index) {
			return data[index];
		}
		const T &operator[](uint16_t index) const {
			return data[index];
		}
		uint16_t size() const {
			return sz;
		}
		bool empty() const {
			return sz == 0;
		}

		// Iterator methods
		T * __restrict__ begin() {
			return data;
		}
		T * __restrict__ end() {
			return data + sz;
		}
		const T * __restrict__ begin() const {
			return data;
		}
		const T * __restrict__ end() const {
			return data + sz;
		}
	};
}
