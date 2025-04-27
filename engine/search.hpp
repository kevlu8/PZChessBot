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
#define RFP_THRESHOLD (150 * CP_SCALE_FACTOR)

// Aspiration window size(s)
// The aspiration window is the range of values we search
// for the best move. If we fail to find the best move in
// this range, we expand the window.
#define ASPIRATION_WINDOW (50 * CP_SCALE_FACTOR)

// Null-move pruning reduction value
// This is the amount of depth we reduce the search by
// when we do a null-move search
#define NMP_R_VALUE 3

// Delta pruning threshold
// This is the threshold for delta pruning (in centipawns)
#define DELTA_THRESHOLD (300 * CP_SCALE_FACTOR)

extern uint64_t nodes;

std::pair<Move, Value> search(Board &board, int64_t time = 1e9, bool quiet = false);

std::pair<Move, Value> search_depth(Board &board, int depth, bool quiet = false);

std::pair<Move, Value> search_nodes(Board &board, uint64_t nodes);

uint64_t perft(Board &board, int depth);
