#pragma once

#include "bitboard.hpp"
#include "eval.hpp"
#include "movegen.hpp"
#include "ttable.hpp"
#include <algorithm>

extern uint64_t nodes;

std::pair<Move, Value> search(Board &board, int64_t depth = -1);

uint64_t perft(Board &board, int depth);
