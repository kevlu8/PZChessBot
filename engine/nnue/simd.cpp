#if defined(__x86_64__) || defined(_M_X64)
#if defined(__AVX512BW__)
#include "avx512.hpp"
#else
#include "avx2.hpp"
#endif
#else
#include "scalar.hpp"
#endif
