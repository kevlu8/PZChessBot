#pragma once

#include "../arch.hpp"

#if defined(TARGET_X86_AVX512)

namespace simd {
	ivec setzero_ivec() {
		return _mm512_setzero_si512();
	}

	fvec setzero_fvec() {
		return _mm512_setzero_ps();
	}

	ivec broadcast_i16(int16_t x) {
		return _mm512_set1_epi16(x);
	}

	fvec broadcast_f32(float x) {
		return _mm512_set1_ps(x);
	}

	ivec load_ivec(const ivec *p) {
		return _mm512_loadu_si512(p);
	}

	fvec load_fvec(const float *p) {
		return _mm512_loadu_ps(p);
	}

	ivec clamp_i16(ivec x, ivec lo, ivec hi) {
		x = _mm512_max_epi16(x, lo);
		x = _mm512_min_epi16(x, hi);
		return x;
	}

	fvec clamp_f32(fvec x, fvec lo, fvec hi) {
		x = _mm512_max_ps(x, lo);
		x = _mm512_min_ps(x, hi);
		return x;
	}

	ivec shift_mulhi(ivec a, ivec b) {
		a = _mm512_slli_epi16(a, 7);
		return _mm512_mulhrs_epi16(a, b);
	}

	ivec accdp_u8i8_i16(ivec a, ivec b, ivec c) {
#if defined(__AVX512VNNI__)
		return _mm512_dpbusd_epi32(c, a, b);
#else
		ivec sum = _mm512_maddubs_epi16(a, b);
		return _mm512_add_epi16(sum, c);
#endif
	}

	fvec cvt_i32_f32(ivec v) {
		return _mm512_cvtepi32_ps(v);
	}

	fvec fma_f32(fvec a, fvec b, fvec c) {
		return _mm512_fmadd_ps(a, b, c);
	}

	fvec mul_f32(fvec a, fvec b) {
		return _mm512_mul_ps(a, b);
	}

	fvec add_f32(fvec a, fvec b) {
		return _mm512_add_ps(a, b);
	}

	void store_f32(float *p, fvec v) {
		_mm512_storeu_ps(p, v);
	}

	void store_u16_u8(uint8_t *p, ivec v) {
		_mm256_storeu_si256((__m256i *)p, _mm512_cvtusepi16_epi8(v));
	}

	float reduce_add_ps(fvec v) {
		__m256 tmp = _mm256_add_ps(_mm512_castps512_ps256(v), _mm512_extractf32x8_ps(v, 1));
		__m128 sum = _mm_add_ps(_mm256_castps256_ps128(tmp), _mm256_extractf128_ps(tmp, 1));
		sum = _mm_add_ps(sum, _mm_movehdup_ps(sum));
		sum = _mm_add_ps(sum, _mm_movehl_ps(sum, sum));

		return _mm_cvtss_f32(sum);
	}

	int32_t reduce_add_epi16(ivec v) {
#if defined(__AVX512VNNI__)
		__m512i wide = v;
#else
		const __m512i ones = _mm512_set1_epi16(1);
		__m512i wide = _mm512_madd_epi16(v, ones);
#endif

		return _mm512_reduce_add_epi32(wide);
	}
}

#endif
