#pragma once

#include "bitboard.hpp"
#include "boardstate.hpp"
#include "includes.hpp"
#include "nnue/accumulator.hpp"
#include "nnue/network.hpp"

extern Network *nnue_networks;

Value simple_eval(Position &);

Value eval(Position &pos, AccumulatorManager &am);

std::array<Value, 8> debug_eval(Position &pos);
