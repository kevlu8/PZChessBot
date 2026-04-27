#pragma once

#include "simd.hpp"

ivec simd::setzero_ivec() {
	return _mm256_setzero_si256();
}

fvec simd::setzero_fvec() {
	return _mm256_setzero_ps();
}

ivec simd::broadcast_i16(int16_t x) {
	return _mm256_set1_epi16(x);
}

fvec simd::broadcast_f32(float x) {
	return _mm256_set1_ps(x);
}

ivec simd::load_ivec(const ivec *p) {
	return _mm256_load_si256(p);
}

fvec simd::load_fvec(const float *p) {
	return _mm256_load_ps(p);
}

ivec simd::max_i16(ivec a, ivec b) {
	return _mm256_max_epi16(a, b);
}

ivec simd::min_i16(ivec a, ivec b) {
	return _mm256_min_epi16(a, b);
}

fvec simd::max_f32(fvec a, fvec b) {
	return _mm256_max_ps(a, b);
}

fvec simd::min_f32(fvec a, fvec b) {
	return _mm256_min_ps(a, b);
}

ivec simd::shift_mulhi(ivec a, ivec b) {
	a = _mm256_slli_epi16(a, 7);
	return _mm256_mulhrs_epi16(a, b);
}

ivec simd::accdp_u8i8_i16(ivec a, ivec b, ivec c) {
	ivec sum = _mm256_maddubs_epi16(a, b);
	return _mm256_add_epi16(sum, c);
}

fvec simd::cvt_i32_f32(ivec v) {
	return _mm256_cvtepi32_ps(v);
}

fvec simd::fma_f32(fvec a, fvec b, fvec c) {
	return _mm256_fmadd_ps(a, b, c);
}

fvec simd::mul_f32(fvec a, fvec b) {
	return _mm256_mul_ps(a, b);
}

void simd::store_f32(float *p, fvec v) {
	_mm256_store_ps(p, v);
}

void simd::store_i16_i8(int8_t *p, ivec v) {
	const __m128i shuf_mask = _mm_cvtsi64_si128(0x0e0c0a0806040200);

	__m128i lo = _mm256_castsi256_si128(v);
	__m128i hi = _mm256_extracti128_si256(v, 1);

	_mm_storeu_si64(&p[0], _mm_shuffle_epi8(lo, shuf_mask));
	_mm_storeu_si64(&p[8], _mm_shuffle_epi8(hi, shuf_mask));
}

float simd::reduce_add_ps(fvec v) {
	__m128 sum = _mm_add_ps(_mm256_castps256_ps128(v), _mm256_extractf128_ps(v, 1));
	sum = _mm_add_ps(sum, _mm_movehdup_ps(sum));
	sum = _mm_add_ps(sum, _mm_movehl_ps(sum, sum));

	return _mm_cvtss_f32(sum);
}

int32_t simd::reduce_add_epi16(ivec v) {
	const __m256i ones = _mm256_set1_epi16(1);

	__m256i wide = _mm256_madd_epi16(v, ones);

	__m128i sum = _mm_add_epi32(_mm256_castsi256_si128(wide), _mm256_extracti128_si256(wide, 1));
	sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, _MM_SHUFFLE(1, 0, 3, 2)));
	sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, _MM_SHUFFLE(2, 3, 0, 1)));

	return _mm_cvtsi128_si32(sum);
}
