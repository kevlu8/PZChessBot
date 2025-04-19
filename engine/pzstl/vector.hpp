#pragma once

#define PZSTL_MAX_SIZE 256

namespace pzstd {
	template <typename T>
	struct vector {
		// It is necessary to write this class using static allocation only
		T data[PZSTL_MAX_SIZE];
		uint16_t sz = 0;

		void push_back(T value) noexcept {
			data[sz++] = value;
		}
		void pop_back() noexcept {
			sz--;
		}
		void clear() {
			sz = 0;
		}
		uint16_t count(T value) const {
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

	// This struct is (temporarily) necessary for storing hash_hist as games may last longer than 256 ply.
	// In theory, once hash_hist is implemented properly, it will never exceed size 50 due to the 50 move rule.
	template<typename T>
	struct largevector {
		T data[1024];
		uint16_t sz = 0;

		void push_back(T value) noexcept {
			data[sz++] = value;
		}
		void pop_back() noexcept {
			sz--;
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