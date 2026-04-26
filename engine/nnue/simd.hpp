#include <immintrin.h>

namespace simd {
	void store_epi16_epi8(int8_t *p, __m256i v);
	int32_t reduce_add_epi16(__m256i v);
};
