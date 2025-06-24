#pragma once

#include "includes.hpp"

#include "nnue/network.hpp"

struct BoardState {
	Accumulator w_acc, b_acc;
	Piece mailbox[64] = {};
	bool refresh = true;
};

extern BoardState bs;