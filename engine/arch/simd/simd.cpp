#include "simd.hpp"

#if defined(TARGET_ARM_NEON)
#include "neon.hpp"
#elif defined(TARGET_X86_AVX512)
#include "avx512.hpp"
#elif defined(TARGET_X86_AVX2)
#include "avx2.hpp"
#else
#include "scalar.hpp"
#endif
