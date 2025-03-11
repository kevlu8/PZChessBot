#pragma once

#include "bitboard.hpp"
#include "eval.hpp"
#include "movegen.hpp"
#include "ttable.hpp"
#include <algorithm>

extern uint64_t nodes;

std::pair<Move, Value> search(Board &board, int64_t time = 1e9);

std::pair<Move, Value> search_depth(Board &board, int depth);

uint64_t perft(Board &board, int depth);
