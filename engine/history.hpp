#pragma once

#include "includes.hpp"

struct ContHistEntry {
	Value hist[2][6][64]; // [side][piecetype][to]

	ContHistEntry() {
		memset(hist, 0, sizeof(hist));
	}
};
