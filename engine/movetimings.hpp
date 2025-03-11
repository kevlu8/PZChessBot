#pragma once

#include "includes.hpp"

uint64_t timemgmt(int64_t remtime, int64_t inc = 0) {
	// Return time in ms that we can spend on this move
	return std::max(1l, remtime / 25 + inc * 3 / 5);
}
