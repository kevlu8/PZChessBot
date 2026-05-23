#pragma once

#include "includes.hpp"

#include "bitboard.hpp"
#include "eval.hpp"
#include "history.hpp"
#include "movegen.hpp"
#include "movepicker.hpp"
#include "tb.hpp"
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
extern bool show_wdl;
extern bool do_softnodes;
extern bool do_datagen;

struct alignas(64) NodeCounter {
	std::atomic<uint64_t> val = 0;

	void operator++(int) {
		val.fetch_add(1, std::memory_order_relaxed);
	}

    void operator=(uint64_t new_val) {
        val.store(new_val, std::memory_order_relaxed);
    }

    uint64_t get() const {
        return val.load(std::memory_order_relaxed);
    }
};

extern NodeCounter nodes[MAX_THREADS];
extern std::unordered_set<uint16_t> tb_moves;

struct alignas(4096) ThreadInfo {
	Position pos;
    RepetitionHandler rp;
	int maxdepth = 0, seldepth = 0;
    Value eval = 0;
    int id = 0;
	bool is_main = false;
    alignas(64) History thread_hist;
    alignas(64) Corrhist thread_corrhist;
	SSEntry line[MAX_PLY + 5] = {};
	Move pvtable[MAX_PLY + 5][MAX_PLY + 5];
	int pvlen[MAX_PLY + 5] = {};
    AccumulatorManager am;
    bool nmp_disable = false;

    ThreadInfo() : am(pos) {}
};

void prepare_search(int64_t time, int64_t maxnodes, bool quiet, uint16_t num_threads);

void iterativedeepening(Position &pos, ThreadInfo &ti, int depth);

uint64_t perft(Position &pos, int depth);

void clear_search_vars(ThreadInfo &ti);
