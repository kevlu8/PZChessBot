#pragma once

#include "includes.hpp"

#include "nnue/network.hpp"

struct BoardState {
	Accumulator w_acc[NINPUTS * 2][NINPUTS * 2], b_acc[NINPUTS * 2][NINPUTS * 2];
	Piece mailbox[NINPUTS * 2][NINPUTS * 2][64] = {};
};

extern BoardState bs[64];
