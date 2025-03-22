#pragma once

#include "bitboard.hpp"
#include "eval.hpp"
#include "movegen.hpp"
#include "ttable.hpp"
#include <algorithm>

// Eval per ply threshold for RFP
// RFP stops searching if our position is so good that
// we can afford to lose RFP_THRESHOLD eval units per ply
// and still be in a better position
#define RFP_THRESHOLD 150

// Aspiration window size(s)
// The aspiration window is the range of values we search
// for the best move. If we fail to find the best move in
// this range, we expand the window.
#define ASPIRATION_WINDOW 50

extern uint64_t nodes;

std::pair<Move, Value> search(Board &board, int64_t time = 1e9);

std::pair<Move, Value> search_depth(Board &board, int depth);

uint64_t perft(Board &board, int depth);
