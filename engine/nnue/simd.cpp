#include <immintrin.h>

namespace simd {

	void store_epi16_epi8(int8_t *p, __m256i v) {
		const __m128i shuf_mask = _mm_cvtsi64_si128(0x0e0c0a0806040200);

		__m128i lo = _mm256_castsi256_si128(v);
		__m128i hi = _mm256_extracti128_si256(v, 1);

		_mm_storeu_si64(&p[0], _mm_shuffle_epi8(lo, shuf_mask));
		_mm_storeu_si64(&p[8], _mm_shuffle_epi8(hi, shuf_mask));
	}

};
