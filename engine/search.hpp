#pragma once

#include "includes.hpp"

#include "bitboard.hpp"
#include "eval.hpp"
#include "history.hpp"
#include "movegen.hpp"
#include "movepicker.hpp"
#include "ttable.hpp"

#include <algorithm>
#include <atomic>
#include <thread>

// Eval per ply threshold for RFP
// RFP stops searching if our position is so good that
// we can afford to lose RFP_THRESHOLD eval units per ply
// and still be in a better position. The lower the value,
// the more aggressive RFP is.
#define RFP_THRESHOLD 131
#define RFP_IMPROVING 30

// Aspiration window size(s)
// The aspiration window is the range of values we search
// for the best move. If we fail to find the best move in
// this range, we expand the window.
#define ASPIRATION_WINDOW 32

// Null-move pruning reduction value
// This is the amount of depth we reduce the search by
// when we do a null-move search
#define NMP_R_VALUE 4

// Delta pruning threshold
// This is the threshold for delta pruning (in centipawns)
#define DELTA_THRESHOLD 300

// Futility pruning threshold
// This is the threshold for futility pruning (in centipawns)
#define FUTILITY_THRESHOLD 304
#define FUTILITY_THRESHOLD2 692

// Razoring margin
// This is the margin for razoring (in centipawns)
#define RAZOR_MARGIN 241

// History pruning margin
// This is the margin for history pruning (in centipawns)
#define HISTORY_MARGIN 4000

extern std::atomic<uint64_t> nodes;
extern bool early_exit;

struct ThreadData {
	Board b;
	Value v;

	History history;

	Move pvtable[MAX_PLY][MAX_PLY];
	int pvlen[MAX_PLY];

	SSEntry line[MAX_PLY];
	uint64_t nodecnt[64][64];

	ThreadData(TTable *tt) : b(tt) {}
};

std::pair<Move, Value> search(Board &board, int64_t time = 1e9, int depth = MAX_PLY, int64_t nodes = 1e18, int quiet = 0, int num_threads = 1);

pzstd::vector<std::pair<Move, Value>> search_multipv(Board &board, int multipv, int64_t time = 1e9, int depth = MAX_PLY, int64_t maxnodes = 1e18, int quiet = 0);

uint64_t perft(Board &board, int depth);

void clear_search_vars();
