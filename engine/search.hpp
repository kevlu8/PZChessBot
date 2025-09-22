#pragma once

#include "bitboard.hpp"
#include "eval.hpp"
#include "movegen.hpp"
#include "ttable.hpp"
#include <algorithm>

// Eval per ply threshold for RFP
// RFP stops searching if our position is so good that
// we can afford to lose RFP_THRESHOLD eval units per ply
// and still be in a better position. The lower the value,
// the more aggressive RFP is.
#define RFP_THRESHOLD (131 * CP_SCALE_FACTOR)
#define RFP_IMPROVING (30 * CP_SCALE_FACTOR)

// Aspiration window size(s)
// The aspiration window is the range of values we search
// for the best move. If we fail to find the best move in
// this range, we expand the window.
#define ASPIRATION_WINDOW (32 * CP_SCALE_FACTOR)

// Null-move pruning reduction value
// This is the amount of depth we reduce the search by
// when we do a null-move search
#define NMP_R_VALUE 4

// Delta pruning threshold
// This is the threshold for delta pruning (in centipawns)
#define DELTA_THRESHOLD (300 * CP_SCALE_FACTOR)

// Futility pruning threshold
// This is the threshold for futility pruning (in centipawns)
#define FUTILITY_THRESHOLD (304 * CP_SCALE_FACTOR)
#define FUTILITY_THRESHOLD2 (692 * CP_SCALE_FACTOR)

// Razoring margin
// This is the margin for razoring (in centipawns)
#define RAZOR_MARGIN (241 * CP_SCALE_FACTOR)

// History pruning margin
// This is the margin for history pruning (in centipawns)
#define HISTORY_MARGIN (4000 * CP_SCALE_FACTOR)

// Correction history table size
#define CORRHIST_SZ 32768
#define CORRHIST_GRAIN 256
#define CORRHIST_WEIGHT 256

extern uint64_t nodes;

struct SSEntry {
	Move move;
	Value eval;
	Move excl;
	SSEntry() : move(NullMove), eval(VALUE_NONE), excl(NullMove) {}
};

std::pair<Move, Value> search(Board &board, int64_t time = 1e9, int depth = MAX_PLY, int64_t nodes = 1e18, int quiet = 0);

pzstd::vector<std::pair<Move, Value>> search_multipv(Board &board, int multipv, int64_t time = 1e9, int depth = MAX_PLY, int64_t maxnodes = 1e18, int quiet = 0);

uint64_t perft(Board &board, int depth);

void clear_search_vars();
