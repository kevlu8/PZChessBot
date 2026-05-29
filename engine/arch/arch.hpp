#pragma once

#include <cstdint>

// Define constants for each target
#if defined(__ARM_NEON)

using ivec = uint8x16_t;
using fvec = float32x4_t;
#define VEC_SIZE 128

#define L1_UNROLL 4
#define L2_UNROLL 4
#define L3_UNROLL 2

#define TARGET_ARM_NEON

#elif defined(__AVX512BW__)

#include <immintrin.h>

using ivec = __m512i;
using fvec = __m512;
#define VEC_SIZE 512

#define L1_UNROLL 4
#define L2_UNROLL 2
#define L3_UNROLL 1

#define TARGET_X86_AVX512

#elif defined(__AVX2__)

#include <immintrin.h>

using ivec = __m256i;
using fvec = __m256;
#define VEC_SIZE 256

#define L1_UNROLL 4
#define L2_UNROLL 4
#define L3_UNROLL 2

#define TARGET_X86_AVX2

#else
#error "Unsupported architecture"
#endif

// Determine pext usage
#if defined(__BMI2__) && !defined(__znver1__) && !defined(__znver2__)
#define USE_PEXT
#endif

#define BYTES_PER_VEC (VEC_SIZE / 8)
#define SHORTS_PER_VEC (VEC_SIZE / 16)
#define FLOATS_PER_VEC (VEC_SIZE / 32)

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
	void prefetcht0(void *p);
}

namespace simd {
	ivec setzero_ivec();
	fvec setzero_fvec();

	ivec broadcast_i16(int16_t x);
	fvec broadcast_f32(float x);

	ivec load_ivec(const ivec *p);
	fvec load_fvec(const float *p);

	ivec clamp_i16(ivec x, ivec lo, ivec hi);
	fvec clamp_f32(fvec x, fvec lo, fvec hi);

	ivec shift_mulhi(ivec a, ivec b);
	ivec accdp_u8i8_i16(ivec a, ivec b, ivec c);

	fvec cvt_i32_f32(ivec v);

	fvec fma_f32(fvec a, fvec b, fvec c);
	fvec mul_f32(fvec a, fvec b);
	fvec add_f32(fvec a, fvec b);

	void store_f32(float *p, fvec v);

	void store_u16_u8(uint8_t *p, ivec v);
	float reduce_add_ps(fvec v);
	int32_t reduce_add_epi16(ivec v);
};
