#pragma once

#include "includes.hpp"

uint64_t timemgmt(int64_t remtime, int64_t inc = 0, bool online = 0) {
	// Return time in ms that we can spend on this move
	if (online && remtime < 5000) return 100;
	return std::max(1ll, (long long)(remtime / 25 + inc * 3 / 5));
}
