#pragma once

#include "bitboard.hpp"
#include "nnue/network.hpp"
#include "includes.hpp"
#include "boardstate.hpp"

extern Network nnue_network;

Value simple_eval(Board &board);

Value eval(Board &board, BoardState *bs);

std::array<Value, 8> debug_eval(Board &board);

constexpr int IBUCKET_LAYOUT[] = {
	0, 0, 2, 2, 3, 3, 1, 1,
	0, 0, 2, 2, 3, 3, 1, 1,
	4, 4, 4, 4, 5, 5, 5, 5,
	4, 4, 4, 4, 5, 5, 5, 5,
	6, 6, 6, 6, 7, 7, 7, 7,
	6, 6, 6, 6, 7, 7, 7, 7,
	6, 6, 6, 6, 7, 7, 7, 7,
	6, 6, 6, 6, 7, 7, 7, 7,
};
