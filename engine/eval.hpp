#pragma once

#include "bitboard.hpp"
#include "nnue/network.hpp"
#include "includes.hpp"
#include "boardstate.hpp"

extern Network nnue_network;

Value simple_eval(Position &);

Value eval(Position &, BoardState *);

std::array<Value, 8> debug_eval(Position &);

constexpr int IBUCKET_LAYOUT[] = {
	0, 2, 4, 6, 7, 5, 3, 1,
	8, 8, 10, 10, 11, 11, 9, 9,
	12, 12, 12, 12, 13, 13, 13, 13,
	12, 12, 12, 12, 13, 13, 13, 13,
	14, 14, 14, 14, 15, 15, 15, 15,
	14, 14, 14, 14, 15, 15, 15, 15,
	14, 14, 14, 14, 15, 15, 15, 15,
	14, 14, 14, 14, 15, 15, 15, 15,
};
