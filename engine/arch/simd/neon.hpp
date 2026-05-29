#pragma once

#include "simd.hpp"

#if defined(TARGET_ARM_NEON)

namespace simd {
	ivec setzero_ivec() {
		return (ivec)vdupq_n_u32(0);
	}

	fvec setzero_fvec() {
		return vdupq_n_f32(0.0f);
	}

	ivec broadcast_i16(int16_t x) {
		return vdupq_n_s16(x);
	}

	fvec broadcast_f32(float x) {
		return vdupq_n_f32(x);
	}

	ivec load_ivec(const ivec *p) {
		return vld1q_s16((int16_t *)p);
	}

	fvec load_fvec(const float *p) {
		return vld1q_f32(p);
	}

	ivec clamp_i16(ivec x, ivec lo, ivec hi) {
		x = vmaxq_s16(x, lo);
		x = vminq_s16(x, hi);
		return x;
	}

	fvec clamp_f32(fvec x, fvec lo, fvec hi) {
		x = vmaxq_f32(x, lo);
		x = vminq_f32(x, hi);
		return x;
	}

	ivec shift_mulhi(ivec a, ivec b) {
		a = vshlq_n_s16(a, 7);
		return vqrdmulhq_s16(a, b);
	}

	ivec accdp_u8i8_i16(ivec a, ivec b, ivec c) {
		int16x8_t a_lo = (int16x8_t)vmovl_u8(vget_low_u8((uint8x16_t)a));
		int16x8_t a_hi = (int16x8_t)vmovl_u8(vget_high_u8((uint8x16_t)a));

		int16x8_t b_lo = (int16x8_t)vmovl_s8(vget_low_s8((int8x16_t)b));
		int16x8_t b_hi = (int16x8_t)vmovl_s8(vget_high_s8((int8x16_t)b));

		int16x8_t sum = vpaddq_s16(vmulq_s16(a_lo, b_lo), vmulq_s16(a_hi, b_hi));
		return vaddq_s16(sum, c);
	}

	fvec cvt_i32_f32(ivec v) {
		return vcvtq_f32_s32((int32x4_t)v);
	}

	fvec fma_f32(fvec a, fvec b, fvec c) {
		return vfmaq_f32(c, a, b);
	}

	fvec mul_f32(fvec a, fvec b) {
		return vmulq_f32(a, b);
	}

	fvec add_f32(fvec a, fvec b) {
		return vaddq_f32(a, b);
	}

	void store_f32(float *p, fvec v) {
		return vst1q_f32(p, v);
	}

	void store_u16_u8(uint8_t *p, ivec v) {
		vst1_u8(p, vqmovn_u16((uint16x8_t)v));
	}

	float reduce_add_ps(fvec v) {
		float32x2_t lo = vget_low_f32(v);
		float32x2_t hi = vget_high_f32(v);

		float32x2_t sum = vpadd_f32(lo, hi);
		sum = vpadd_f32(sum, sum);
		return vget_lane_f32(sum, 0);
	}

	int32_t reduce_add_epi16(ivec v) {
		return vaddlvq_s16(v);
	}
}

#endif
