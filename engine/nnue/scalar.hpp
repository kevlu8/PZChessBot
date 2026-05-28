#pragma once

#if !defined(__x86_64__) && !defined(_M_X64)

#include "simd.hpp"
#include <algorithm>
#include <cstring>

ivec simd::setzero_ivec() {
    ivec r;
    memset(&r, 0, sizeof(r));
    return r;
}

fvec simd::setzero_fvec() {
    fvec r;
    for (int i = 0; i < 8; i++) r.f32[i] = 0.0f;
    return r;
}

ivec simd::broadcast_i16(int16_t x) {
    ivec r;
    for (int i = 0; i < 16; i++) r.i16[i] = x;
    return r;
}

fvec simd::broadcast_f32(float x) {
    fvec r;
    for (int i = 0; i < 8; i++) r.f32[i] = x;
    return r;
}

ivec simd::load_ivec(const ivec *p) {
    ivec r;
    memcpy(&r, p, sizeof(r));
    return r;
}

fvec simd::load_fvec(const float *p) {
    fvec r;
    memcpy(&r, p, sizeof(r));
    return r;
}

ivec simd::clamp_i16(ivec x, ivec lo, ivec hi) {
    ivec r;
    for (int i = 0; i < 16; i++)
        r.i16[i] = std::max(lo.i16[i], std::min(hi.i16[i], x.i16[i]));
    return r;
}

fvec simd::clamp_f32(fvec x, fvec lo, fvec hi) {
    fvec r;
    for (int i = 0; i < 8; i++)
        r.f32[i] = std::max(lo.f32[i], std::min(hi.f32[i], x.f32[i]));
    return r;
}

// Emulates _mm256_mulhrs_epi16 applied after shifting a left by 7:
// result = (int16_t)(((int32_t)(a << 7) * b + 0x4000) >> 15)
ivec simd::shift_mulhi(ivec a, ivec b) {
    ivec r;
    for (int i = 0; i < 16; i++) {
        int32_t av = (int32_t)a.i16[i] << 7;
        r.i16[i] = (int16_t)((av * (int32_t)b.i16[i] + 0x4000) >> 15);
    }
    return r;
}

// Emulates _mm256_maddubs_epi16: pairs of (u8*i8) summed into i16, then added to c
ivec simd::accdp_u8i8_i16(ivec a, ivec b, ivec c) {
    ivec r;
    for (int i = 0; i < 16; i++) {
        int32_t sum = (int32_t)(uint8_t)a.u8[2*i] * (int32_t)(int8_t)b.i8[2*i]
                    + (int32_t)(uint8_t)a.u8[2*i+1] * (int32_t)(int8_t)b.i8[2*i+1];
        // saturate to int16 range
        sum = std::max(-32768, std::min(32767, sum));
        r.i16[i] = (int16_t)(sum + c.i16[i]);
    }
    return r;
}

fvec simd::cvt_i32_f32(ivec v) {
    fvec r;
    for (int i = 0; i < 8; i++) r.f32[i] = (float)v.i32[i];
    return r;
}

fvec simd::fma_f32(fvec a, fvec b, fvec c) {
    fvec r;
    for (int i = 0; i < 8; i++) r.f32[i] = a.f32[i] * b.f32[i] + c.f32[i];
    return r;
}

fvec simd::mul_f32(fvec a, fvec b) {
    fvec r;
    for (int i = 0; i < 8; i++) r.f32[i] = a.f32[i] * b.f32[i];
    return r;
}

fvec simd::add_f32(fvec a, fvec b) {
    fvec r;
    for (int i = 0; i < 8; i++) r.f32[i] = a.f32[i] + b.f32[i];
    return r;
}

void simd::store_f32(float *p, fvec v) {
    memcpy(p, v.f32, sizeof(v.f32));
}

// Emulates _mm256_packus_epi16 then store: saturate u16->u8, store 16 bytes
void simd::store_u16_u8(uint8_t *p, ivec v) {
    for (int i = 0; i < 16; i++) {
        uint16_t val = v.u16[i];
        p[i] = (uint8_t)(val > 255 ? 255 : val);
    }
}

float simd::reduce_add_ps(fvec v) {
    float sum = 0.0f;
    for (int i = 0; i < 8; i++) sum += v.f32[i];
    return sum;
}

int32_t simd::reduce_add_epi16(ivec v) {
    int32_t sum = 0;
    for (int i = 0; i < 16; i++) sum += (int32_t)v.i16[i];
    return sum;
}

#endif
