#pragma once

#include "bitboard.hpp"
#include "nnue/network.hpp"
#include "includes.hpp"
#include "boardstate.hpp"
#include "nnue/accumulator.hpp"

extern Network nnue_network;

Value simple_eval(Position &);

Value eval(Position &pos, AccumulatorManager &am);

std::array<Value, 8> debug_eval(Position &pos);
