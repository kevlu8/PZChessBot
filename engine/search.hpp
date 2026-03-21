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
#define RFP_THRESHOLD 60
#define RFP_QUADRATIC 5
#define RFP_IMPROVING 31
#define RFP_CUTNODE 18

// ProbCut margin
#define PROBCUT_MARGIN 370

// Aspiration window size(s)
// The aspiration window is the range of values we search
// for the best move. If we fail to find the best move in
// this range, we expand the window.
#define ASPIRATION_WINDOW 16

// Null-move pruning reduction value
// This is the amount of depth we reduce the search by
// when we do a null-move search
#define NMP_R_VALUE 7

// Delta pruning threshold
#define DELTA_THRESHOLD 355

// Razoring margin
#define RAZOR_MARGIN 244

// History pruning margin
#define HISTORY_MARGIN 2753

extern bool stop_search;

extern std::atomic<uint64_t> nodes[MAX_THREADS];

struct ThreadInfo {
	Board board;
	int maxdepth = 0, seldepth = 0;
    Value eval = 0;
    int id = 0;
	bool is_main = false;
    alignas(64) History thread_hist;
	SSEntry line[MAX_PLY + 5] = {};
	Move pvtable[MAX_PLY + 5][MAX_PLY + 5];
	int pvlen[MAX_PLY + 5] = {};
	BoardState bs[NINPUTS * 2][NINPUTS * 2];
    bool nmp_disable = false;

    ThreadInfo() {
        set_bs();
    }

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

void prepare_search(int64_t time, int64_t maxnodes, bool quiet, uint16_t num_threads);

void iterativedeepening(ThreadInfo &ti, int depth);

uint64_t perft(Board &board, int depth);

void clear_search_vars(ThreadInfo &ti);
