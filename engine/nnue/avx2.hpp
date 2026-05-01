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

ivec simd::broadcast_i32(int32_t x) {
	return _mm256_set1_epi32(x);
}

fvec simd::broadcast_f32(float x) {
	return _mm256_set1_ps(x);
}

ivec simd::load_ivec(const ivec *p) {
	return _mm256_loadu_si256(p);
}

fvec simd::load_fvec(const float *p) {
	return _mm256_loadu_ps(p);
}

ivec simd::clamp_i16(ivec x, ivec lo, ivec hi) {
	x = _mm256_max_epi16(x, lo);
	x = _mm256_min_epi16(x, hi);
	return x;
}

fvec simd::clamp_f32(fvec x, fvec lo, fvec hi) {
	x = _mm256_max_ps(x, lo);
	x = _mm256_min_ps(x, hi);
	return x;
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

fvec simd::add_f32(fvec a, fvec b) {
	return _mm256_add_ps(a, b);
}

ivec simd::add_i32(ivec a, ivec b) {
	return _mm256_add_epi32(a, b);
}

void simd::store_ivec(ivec *p, ivec v) {
	_mm256_storeu_si256(p, v);
}

void simd::store_f32(float *p, fvec v) {
	_mm256_storeu_ps(p, v);
}

void simd::store_u16_u8(uint8_t *p, ivec v) {
	__m256i res = _mm256_packus_epi16(v, v);
	res = _mm256_permute4x64_epi64(res, _MM_PERM_DCCA);

	_mm_storeu_si128((__m128i *)p, _mm256_castsi256_si128(res));
}

float simd::reduce_add_ps(fvec v) {
	__m128 sum = _mm_add_ps(_mm256_castps256_ps128(v), _mm256_extractf128_ps(v, 1));
	sum = _mm_add_ps(sum, _mm_movehdup_ps(sum));
	sum = _mm_add_ps(sum, _mm_movehl_ps(sum, sum));

	return _mm_cvtss_f32(sum);
}

ivec simd::hadd_i16_i32(ivec v) {
	const __m256i ones = _mm256_set1_epi16(1);
	return _mm256_madd_epi16(v, ones);
}

uint16_t simd::nz_mask(uint8_t *p) {
	const __m256i zero = _mm256_setzero_si256();

	__m256i v = _mm256_loadu_si256((__m256i *)p);
	__m256i msk = _mm256_cmpgt_epi32(v, zero);
	msk = _mm256_or_si256(v, msk);
	return (uint16_t)_mm256_movemask_ps((fvec)msk);
}
