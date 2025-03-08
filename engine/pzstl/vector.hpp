#pragma once

#define PZSTL_MAX_SIZE 256

namespace pzstd {
	template <typename T>
	struct vector {
		// It is necessary to write this class using static allocation only
		T data[PZSTL_MAX_SIZE];
		uint16_t sz = 0;

		void push_back(T value) noexcept {
			if (sz < PZSTL_MAX_SIZE) {
				data[sz++] = value;
			}
		}
		void pop_back() noexcept {
			if (sz > 0) {
				sz--;
			}
		}
		void clear() {
			sz = 0;
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