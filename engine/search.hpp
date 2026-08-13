/*
 * PZChessBot, a UCI chess engine
 * Copyright (C) 2026 Kevin Lu and William Ma
 *
 * PZChessBot is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PZChessBot is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with PZChessBot. If not, see <https://www.gnu.org/licenses/>.
 */

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

// Null-move pruning reduction value
// This is the amount of depth we reduce the search by
// when we do a null-move search
#define NMP_R_VALUE 7

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
	SSEntry *ss;
	int maxdepth = 0, seldepth = 0;
	int id = 0;
	Value eval = 0;
	bool is_main = false;
	RepetitionHandler rp;
	alignas(64) History thread_hist;
	alignas(64) Corrhist thread_corrhist;
	Move pvtable[MAX_PLY + 5][MAX_PLY + 5];
	int pvlen[MAX_PLY + 5] = {};
	AccumulatorManager am;
	bool nmp_disable = false;

	ThreadInfo() : am(pos) {
		ss = (new SSEntry[MAX_PLY + 16]) + 8;
	}

	~ThreadInfo() {
		delete[] (ss - 8);
	}
};

void prepare_search(int64_t time, int64_t maxnodes, bool quiet, uint16_t num_threads);

void iterativedeepening(Position &pos, ThreadInfo &ti, int depth);

uint64_t perft(Position &pos, int depth);

void clear_search_vars(ThreadInfo &ti);
