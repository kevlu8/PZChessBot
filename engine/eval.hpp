#pragma once

#include "bitboard.hpp"
#include "boardstate.hpp"
#include "includes.hpp"
#include "nnue/accumulator.hpp"
#include "nnue/network.hpp"

extern Network nnue_network;

Value simple_eval(Position &);

Value eval(Position &pos, AccumulatorManager &am, const Network &net);

std::array<Value, 8> debug_eval(Position &pos);
