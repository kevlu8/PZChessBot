#pragma once

#include "includes.hpp"

uint64_t timetonodes(uint64_t remtime, bool online = false) {
	// Note: These values are calibrated with 2M nodes per second
	if (!online)
		return remtime * 200;
	if (remtime > 5 * 60 * 1000)
		return remtime * 200;
	if (remtime > 60 * 1000)
		return remtime * 100;
	if (remtime > 15 * 1000)
		return remtime * 80;
	return 100'000; // Literally just play a move so we don't flag
}
