#pragma once

#include "includes.hpp"

#include "bitboard.hpp"
#include "eval.hpp"
#include "history.hpp"
#include "movegen.hpp"
#include "movepicker.hpp"
#include "ttable.hpp"

#include <algorithm>

// Eval per ply threshold for RFP
// RFP stops searching if our position is so good that
// we can afford to lose RFP_THRESHOLD eval units per ply
// and still be in a better position. The lower the value,
// the more aggressive RFP is.
#define RFP_THRESHOLD 62
#define RFP_QUADRATIC 5
#define RFP_IMPROVING 36
#define RFP_CUTNODE 15

// Aspiration window size(s)
// The aspiration window is the range of values we search
// for the best move. If we fail to find the best move in
// this range, we expand the window.
#define ASPIRATION_WINDOW 24

// Null-move pruning reduction value
// This is the amount of depth we reduce the search by
// when we do a null-move search
#define NMP_R_VALUE 4

// Delta pruning threshold
// This is the threshold for delta pruning (in centipawns)
#define DELTA_THRESHOLD 348

// Futility pruning threshold
// This is the threshold for futility pruning (in centipawns)
#define FUTILITY_THRESHOLD 304
#define FUTILITY_THRESHOLD2 692

// Razoring margin
// This is the margin for razoring (in centipawns)
#define RAZOR_MARGIN 267

// History pruning margin
// This is the margin for history pruning (in centipawns)
#define HISTORY_MARGIN 2633

extern bool stop_search;

extern std::atomic<uint64_t> nodes[MAX_THREADS];
extern uint16_t num_threads;

struct ThreadInfo {
	Board board;
	int maxdepth = 0, seldepth = 0;
    Value eval = 0;
    int id = 0;
	bool is_main = false;
	History thread_hist;
	SSEntry line[MAX_PLY] = {};
	Move pvtable[MAX_PLY][MAX_PLY];
	int pvlen[MAX_PLY] = {};
	BoardState bs[NINPUTS * 2][NINPUTS * 2];
    bool nmp_disable = false;

    void set_bs() {
        for (int i = 0; i < NINPUTS * 2; i++) {
            for (int j = 0; j < NINPUTS * 2; j++) {
                for (int k = 0; k < HL_SIZE; k++) {
                    bs[i][j].w_acc.val[k] = nnue_network.accumulator_biases[k];
                    bs[i][j].b_acc.val[k] = nnue_network.accumulator_biases[k];
                }
                for (int k = 0; k < 64; k++) {
                    bs[i][j].mailbox[k] = NO_PIECE;
                }
            }
        }
    }
};

std::pair<Move, Value> search(Board &board, ThreadInfo *threads, int64_t time = 1e9, int depth = MAX_PLY, int64_t nodes = 1e18, int quiet = 0);

uint64_t perft(Board &board, int depth);

void clear_search_vars(ThreadInfo &ti);
