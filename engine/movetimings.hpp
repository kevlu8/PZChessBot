#pragma once

#include "includes.hpp"

TUNE(remtime_mult, 10, 1, 20, 1, 0.002);
TUNE(inc_mult, 80, 1, 120, 5, 0.002);

uint64_t timemgmt(int64_t remtime, int64_t inc = 0, bool online = 0) {
	// Return time in ms that we can spend on this move
	if (online && remtime < 5000) return 100;
	return std::max(1ll, (long long)(remtime * remtime_mult / 100.0 + inc * inc_mult / 100.0));
}
