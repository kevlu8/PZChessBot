#pragma once

#include "bitboard.hpp"
#include "nnue/network.hpp"
#include "history.hpp"
#include "includes.hpp"
#include "boardstate.hpp"

#include <condition_variable>

extern Network nnue_network;

struct ThreadData {
	std::mutex m;
	std::condition_variable cv;
	bool runnable = false;
	bool completed = false;

	Board b;
	int d;
	Value alpha, beta;

	Value v;

	History history;

	Move pvtable[MAX_PLY][MAX_PLY];
	int pvlen[MAX_PLY];

	SSEntry line[MAX_PLY];
	uint64_t nodecnt[64][64];

	BoardState bs[NINPUTS * 2][NINPUTS * 2];

	void reset_bs() {
		for (int j = 0; j < NINPUTS * 2; j++) {
			for (int k = 0; k < NINPUTS * 2; k++) {
				for (int i = 0; i < HL_SIZE; i++) {
					bs[j][k].w_acc.val[i] = nnue_network.accumulator_biases[i];
					bs[j][k].b_acc.val[i] = nnue_network.accumulator_biases[i];
				}
				for (int i = 0; i < 64; i++) bs[j][k].mailbox[i] = NO_PIECE;
			}
		}
	}

	ThreadData(TTable *tt) : b(tt) {
		reset_bs();
	}
};

Value simple_eval(Board &board);

Value eval(Board &board, ThreadData *tdata);

std::array<Value, 8> debug_eval(Board &board);

#ifdef HCE
constexpr int pawn_heatmap[64] = {
	//  a  b  c  d  e  f  g  h
	0,	0,	0,	 0,	  0,   0,	0,	0, // 1
	5,	10, 10,	 -40, -40, 10,	10, 5, // 2
	5,	-5, -10, 0,	  0,   -10, -5, 5, // 3
	0,	0,	0,	 30,  30,  0,	0,	0, // 4
	5,	5,	10,	 40,  40,  10,	5,	5, // 5
	10, 10, 50,	 60,  60,  50,	10, 10, // 6
	80, 80, 80,	 80,  80,  80,	80, 80, // 7
	0,	0,	0,	 0,	  0,   0,	0,	0, // 8
};

constexpr int knight_heatmap[64] = {
	//  a  b  c  d  e  f  g  h
	-50, -40, -30, -30, -30, -30, -40, -50, // 1
	-40, -20, 0,   5,	5,	 0,	  -20, -40, // 2
	-30, 5,	  10,  15,	15,	 10,  5,   -30, // 3
	-30, 0,	  15,  20,	20,	 15,  0,   -30, // 4
	-30, 5,	  15,  20,	20,	 15,  5,   -30, // 5
	-30, 0,	  10,  15,	15,	 10,  0,   -30, // 6
	-40, -20, 0,   0,	0,	 0,	  -20, -40, // 7
	-50, -40, -30, -30, -30, -30, -40, -50, // 8
};

constexpr int bishop_heatmap[64] = {
	//  a  b  c  d  e  f  g  h
	-20, -10, -10, -10, -10, -10, -10, -20, // 1
	-10, 5,	  0,   0,	0,	 0,	  5,   -10, // 2
	-10, 10,  10,  10,	10,	 10,  10,  -10, // 3
	-10, 0,	  10,  10,	10,	 10,  0,   -10, // 4
	-10, 5,	  5,   10,	10,	 5,	  5,   -10, // 5
	-10, 0,	  5,   10,	10,	 5,	  0,   -10, // 6
	-30, 0,	  0,   0,	0,	 0,	  0,   -30, // 7
	-20, -10, -10, -10, -10, -10, -10, -20, // 8
};

constexpr int rook_heatmap[64] = {
	//  a  b  c  d  e  f  g  h
	-10, 0, 0, 10, 10, 5, 0, -10, // 1
	-5,	 0, 0, 0,  0,  0, 0, -5, // 2
	-5,	 0, 0, 0,  0,  0, 0, -5, // 3
	-5,	 0, 0, 0,  0,  0, 0, -5, // 4
	-5,	 0, 0, 0,  0,  0, 0, -5, // 5
	-5,	 0, 0, 0,  0,  0, 0, -5, // 6
	-10, 0, 0, 0,  0,  0, 0, -10, // 7
	0,	 0, 0, 0,  0,  0, 0, 0, // 8
};

constexpr int queen_heatmap[64] = {
	//  a  b  c  d  e  f  g  h
	-20, -10, -10, -5, -5, -10, -10, -20, // 1
	-10, 0,	  5,   0,  0,  0,	0,	 -10, // 2
	-10, 5,	  5,   5,  5,  5,	0,	 -10, // 3
	-5,	 0,	  5,   5,  5,  5,	0,	 -5, // 4
	0,	 0,	  5,   5,  5,  5,	0,	 -5, // 5
	-10, 0,	  5,   5,  5,  5,	0,	 -10, // 6
	-10, 0,	  0,   0,  0,  0,	0,	 -10, // 7
	-20, -10, -10, -5, -5, -10, -10, -20, // 8
};

constexpr int king_heatmap[64] = {
	//  a  b  c  d  e  f  g  h
	30,	 50,  40,  0,	0,	 10,  50,  30, // 1
	20,	 20,  -5,  -5,	-5,	 -5,  20,  20, // 2
	-10, -20, -20, -20, -20, -20, -20, -10, // 3
	-20, -30, -30, -40, -40, -30, -30, -20, // 4
	-30, -40, -40, -50, -50, -40, -40, -30, // 5
	-30, -40, -40, -50, -50, -40, -40, -30, // 6
	-30, -40, -40, -50, -50, -40, -40, -30, // 7
	-30, -40, -40, -50, -50, -40, -40, -30, // 8
};

constexpr int endgame_heatmap[64] = {
	//  a  b  c  d  e  f  g  h
	1, 2,  4,  8,  8,  4,  2,  1, // 1
	2, 4,  8,  16, 16, 8,  4,  2, // 2
	4, 8,  16, 32, 32, 16, 8,  4, // 3
	8, 16, 32, 64, 64, 32, 16, 8, // 4
	8, 16, 32, 64, 64, 32, 16, 8, // 5
	4, 8,  16, 32, 32, 16, 8,  4, // 6
	2, 4,  8,  16, 16, 8,  4,  2, // 7
	1, 2,  4,  8,  8,  4,  2,  1, // 8
};

constexpr int pawn_endgame[64] = {
	//  a  b  c  d  e  f  g  h
	0,	 0,	  0,   0,	0,	 0,	  0,   0, // 1
	-10, -10, -10, -10, -10, -10, -10, -10, // 2
	5,	 5,	  5,   5,	5,	 5,	  5,   5, // 3
	10,	 10,  10,  10,	10,	 10,  10,  10, // 4
	20,	 20,  20,  20,	20,	 20,  20,  20, // 5
	60,	 60,  60,  60,	60,	 60,  60,  60, // 6
	100, 100, 100, 100, 100, 100, 100, 100, // 7
	0,	 0,	  0,   0,	0,	 0,	  0,   0, // 8
};
#else
constexpr int IBUCKET_LAYOUT[] = {
	0, 2, 4, 6, 7, 5, 3, 1,
	8, 8, 10, 10, 11, 11, 9, 9,
	12, 12, 12, 12, 13, 13, 13, 13,
	12, 12, 12, 12, 13, 13, 13, 13,
	14, 14, 14, 14, 15, 15, 15, 15,
	14, 14, 14, 14, 15, 15, 15, 15,
	14, 14, 14, 14, 15, 15, 15, 15,
	14, 14, 14, 14, 15, 15, 15, 15,
};
#endif