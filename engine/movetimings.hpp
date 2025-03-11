#pragma once

#include "includes.hpp"

uint64_t timemgmt(int64_t remtime, int64_t inc = 0) {
	// Return time in ms that we can spend on this move
	if (remtime < 10000) {
		// We are very low on time, just play a move
		return 200;
	}
	// If we are playing online, subtract 300 for ping
	return std::max(1l, remtime / 25 + inc * 3 / 5);
}
