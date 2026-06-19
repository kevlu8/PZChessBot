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

	ivec accdp_u8i8_i32(ivec a, ivec b, ivec c) {
		int16x8_t a_lo = (int16x8_t)vmovl_u8(vget_low_u8((uint8x16_t)a));
		int16x8_t a_hi = (int16x8_t)vmovl_u8(vget_high_u8((uint8x16_t)a));

		int16x8_t b_lo = (int16x8_t)vmovl_s8(vget_low_s8((int8x16_t)b));
		int16x8_t b_hi = (int16x8_t)vmovl_s8(vget_high_s8((int8x16_t)b));

		int32x4_t acc = (int32x4_t)c;
		acc = vmlal_s16(acc, vget_low_s16(a_lo), vget_low_s16(b_lo));
		acc = vmlal_s16(acc, vget_high_s16(a_lo), vget_high_s16(b_lo));
		acc = vmlal_s16(acc, vget_low_s16(a_hi), vget_low_s16(b_hi));
		acc = vmlal_s16(acc, vget_high_s16(a_hi), vget_high_s16(b_hi));
		return (ivec)acc;
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

	int32_t reduce_add_epi32(ivec v) {
		return vaddvq_s32((int32x4_t)v);
	}
}

#endif
