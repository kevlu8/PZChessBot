#pragma once

#include "bitboard.hpp"
#include "eval.hpp"
#include "movegen.hpp"
#include "ttable.hpp"
#include "tunable.hpp"
#include <algorithm>

// Correction history table size (these are not tunable as they are data structure sizes)
#define CORRHIST_SZ 32768
#define CORRHIST_GRAIN 256
#define CORRHIST_WEIGHT 256

// All other search constants are now defined in tunable.hpp and can be adjusted via UCI

extern uint64_t nodes;

struct SSEntry {
	Move move;
	Value eval;
	Move excl;
	SSEntry() : move(NullMove), eval(VALUE_NONE), excl(NullMove) {}
};

std::pair<Move, Value> search(Board &board, int64_t time = 1e9, bool quiet = false);

std::pair<Move, Value> search_depth(Board &board, int depth, bool quiet = false);

std::pair<Move, Value> search_nodes(Board &board, uint64_t nodes);

uint64_t perft(Board &board, int depth);
