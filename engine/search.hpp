#pragma once

#include "bitboard.hpp"
#include "eval.hpp"
#include "movegen.hpp"
#include "ttable.hpp"
#include "tunable.hpp"
#include <algorithm>

// Dynamic tunable parameters - these are now defined in tunable.hpp
// Access them using the global tunable variables

// Delta pruning threshold (still hardcoded as it's not commonly tuned)
#define DELTA_THRESHOLD (300 * CP_SCALE_FACTOR)

// Correction history table size (structural constants, not tunable)
#define CORRHIST_SZ 32768
#define CORRHIST_GRAIN 256
#define CORRHIST_WEIGHT 256

extern uint64_t nodes;

std::pair<Move, Value> search(Board &board, int64_t time = 1e9, bool quiet = false);

std::pair<Move, Value> search_depth(Board &board, int depth, bool quiet = false);

std::pair<Move, Value> search_nodes(Board &board, uint64_t nodes);

uint64_t perft(Board &board, int depth);
