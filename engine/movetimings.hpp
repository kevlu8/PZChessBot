#pragma once

#include "includes.hpp"

uint64_t timetonodes(uint64_t remtime, bool online = false) {
	// Note: These values are calibrated with 5M nodes per second
	if (!online)
		return remtime * 500;
	if (remtime > 5 * 60 * 1000)
		return remtime * 1000;
	if (remtime > 60 * 1000)
		return remtime * 500;
	if (remtime > 15 * 1000)
		return remtime * 300;
	return 100'000; // Literally just play a move so we don't flag
}
