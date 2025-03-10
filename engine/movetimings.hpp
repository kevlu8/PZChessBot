#pragma once

#include "includes.hpp"

uint64_t timemgmt(int64_t remtime, int64_t inc = 0, bool online = false) {
	// Return time in ms that we can spend on this move
	// If we are playing online, subtract 200 for ping
	return std::max(1l, remtime / 25 + inc * 4 / 5 - (online ? 200 : 0));
}
