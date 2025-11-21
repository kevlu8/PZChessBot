#pragma once

#include "includes.hpp"

#include "nnue/network.hpp"

struct BoardState {
	Accumulator w_acc, b_acc;
	Piece mailbox[64] = {};
};

extern BoardState bs[MAX_THREADS][NINPUTS * 2][NINPUTS * 2];