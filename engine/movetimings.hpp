#pragma once

#include "includes.hpp"

uint64_t timetonodes(uint64_t remtime, uint64_t inc = 0, bool online = false) {
	// Note: These values are calibrated with 1M nodes per second
	// * 100 = 10% of time
	// * 40 = 4% of time
	if (!online)
		return remtime * 180;
	if (remtime > 5 * 60 * 1000)
		return remtime * 100;
	if (remtime > 3 * 60 * 1000)
		return remtime * 80;
	if (remtime > 60 * 1000)
		return remtime * 60;
	if (remtime > 15 * 1000)
		return remtime * 40;
	return 100'000; // Literally just play a move so we don't flag
}
