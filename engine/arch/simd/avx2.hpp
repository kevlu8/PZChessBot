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

#include "simd.hpp"

#if defined(TARGET_X86_AVX2)

namespace simd {
	ivec setzero_ivec() {
		return _mm256_setzero_si256();
	}

	fvec setzero_fvec() {
		return _mm256_setzero_ps();
	}

	ivec broadcast_i16(int16_t x) {
		return _mm256_set1_epi16(x);
	}

	fvec broadcast_f32(float x) {
		return _mm256_set1_ps(x);
	}

	ivec load_ivec(const ivec *p) {
		return _mm256_loadu_si256(p);
	}

	fvec load_fvec(const float *p) {
		return _mm256_loadu_ps(p);
	}

	ivec clamp_i16(ivec x, ivec lo, ivec hi) {
		x = _mm256_max_epi16(x, lo);
		x = _mm256_min_epi16(x, hi);
		return x;
	}

	fvec clamp_f32(fvec x, fvec lo, fvec hi) {
		x = _mm256_max_ps(x, lo);
		x = _mm256_min_ps(x, hi);
		return x;
	}

	ivec shift_mulhi(ivec a, ivec b) {
		a = _mm256_slli_epi16(a, 7);
		return _mm256_mulhrs_epi16(a, b);
	}

	ivec accdp_u8i8_i16(ivec a, ivec b, ivec c) {
		ivec sum = _mm256_maddubs_epi16(a, b);
		return _mm256_add_epi16(sum, c);
	}

	fvec cvt_i32_f32(ivec v) {
		return _mm256_cvtepi32_ps(v);
	}

	fvec fma_f32(fvec a, fvec b, fvec c) {
		return _mm256_fmadd_ps(a, b, c);
	}

	fvec mul_f32(fvec a, fvec b) {
		return _mm256_mul_ps(a, b);
	}

	fvec add_f32(fvec a, fvec b) {
		return _mm256_add_ps(a, b);
	}

	void store_f32(float *p, fvec v) {
		_mm256_storeu_ps(p, v);
	}

	void store_u16_u8(uint8_t *p, ivec v) {
		__m256i res = _mm256_packus_epi16(v, v);
		res = _mm256_permute4x64_epi64(res, _MM_PERM_DCCA);

		_mm_storeu_si128((__m128i *)p, _mm256_castsi256_si128(res));
	}

	float reduce_add_ps(fvec v) {
		__m128 sum = _mm_add_ps(_mm256_castps256_ps128(v), _mm256_extractf128_ps(v, 1));
		sum = _mm_add_ps(sum, _mm_movehdup_ps(sum));
		sum = _mm_add_ps(sum, _mm_movehl_ps(sum, sum));

		return _mm_cvtss_f32(sum);
	}

	int32_t reduce_add_epi16(ivec v) {
		const __m256i ones = _mm256_set1_epi16(1);

		__m256i wide = _mm256_madd_epi16(v, ones);

		__m128i sum = _mm_add_epi32(_mm256_castsi256_si128(wide), _mm256_extracti128_si256(wide, 1));
		sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, _MM_SHUFFLE(1, 0, 3, 2)));
		sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, _MM_SHUFFLE(2, 3, 0, 1)));

		return _mm_cvtsi128_si32(sum);
	}
}

#endif
