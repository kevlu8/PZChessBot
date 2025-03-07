#pragma once

#include "includes.hpp"

uint64_t timetonodes(int remtime, bool online=false) {
	// Note: These values are calibrated with 10M nodes per second
	if (!online || remtime > 15*1000) return remtime * 400;
	return 1; // Literally just play a move so we don't flag
}