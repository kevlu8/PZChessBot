#pragma once

#include <immintrin.h>

#if defined(__AVX512BW__)
using ivec = __m512i;
using fvec = __m512;
#define VEC_SIZE 512
#else
using ivec = __m256i;
using fvec = __m256;
#define VEC_SIZE 256
#endif

#define BYTES_PER_VEC (VEC_SIZE / 8)
#define SHORTS_PER_VEC (VEC_SIZE / 16)
#define FLOATS_PER_VEC (VEC_SIZE / 32)

namespace simd {
	ivec setzero_ivec();
	fvec setzero_fvec();

	ivec broadcast_i16(int16_t x);
	fvec broadcast_f32(float x);

	ivec load_ivec(const ivec *p);
	fvec load_fvec(const float *p);

	ivec max_i16(ivec a, ivec b);
	ivec min_i16(ivec a, ivec b);
	fvec max_f32(fvec a, fvec b);
	fvec min_f32(fvec a, fvec b);

	ivec shift_mulhi(ivec a, ivec b);
	ivec accdp_u8i8_i16(ivec a, ivec b, ivec c);

	fvec cvt_i32_f32(ivec v);

	fvec fma_f32(fvec a, fvec b, fvec c);
	fvec mul_f32(fvec a, fvec b);

	void store_f32(float *p, fvec v);

	void store_u16_u8(int8_t *p, ivec v);
	float reduce_add_ps(fvec v);
	int32_t reduce_add_epi16(ivec v);
};
