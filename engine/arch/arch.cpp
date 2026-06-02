#include "arch.hpp"

#include "common.hpp"

#if defined(__aarch64__)
#include "aarch64.hpp"
#elif defined(__amd64__)
#include "amd64.hpp"
#else
#error Unsupported architecture
#endif
