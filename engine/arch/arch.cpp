#include "arch.hpp"

#if defined(__ARM_NEON)
#elif defined(__AVX512BW__)
	#include "avx512.hpp"
#elif defined(__AVX2__)
	#include "avx2.hpp"
#endif
