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

// Aspiration window size(s)
// The aspiration window is the range of values we search
// for the best move. If we fail to find the best move in
// this range, we expand the window.
#define ASPIRATION_WINDOW (36 * CP_SCALE_FACTOR)

// Null-move pruning reduction value
// This is the amount of depth we reduce the search by
// when we do a null-move search
#define NMP_R_VALUE 4

// Delta pruning threshold
// This is the threshold for delta pruning (in centipawns)
#define DELTA_THRESHOLD (300 * CP_SCALE_FACTOR)

// Futility pruning threshold
// This is the threshold for futility pruning (in centipawns)
#define FUTILITY_THRESHOLD (312 * CP_SCALE_FACTOR)
#define FUTILITY_THRESHOLD2 (678 * CP_SCALE_FACTOR)

// Correction history table size
#define CORRHIST_SZ 32768
#define CORRHIST_GRAIN 256
#define CORRHIST_WEIGHT 256

struct SearchParams {
	uint64_t nodes = 0;
	int seldepth = 0;
	uint64_t mx_nodes = 1e18;
	uint64_t mxtime = 1000;
	bool early_exit = false, exit_allowed = false;
	clock_t start = 0;
	Move killer[2][MAX_PLY];
	Value history[2][64][64]; // [side][src][dst]
	Value capthist[6][6][64]; // [piece][captured piece][dst]
	Value corrhist_ps[2][CORRHIST_SZ]; // [side][pawn hash]
	Value corrhist_mat[2][CORRHIST_SZ]; // [side][material hash]
	Move cmh[2][64][64]; // [side][src][dst]
	Move line[MAX_PLY]; // Currently searched line
	Move pvtable[MAX_PLY][MAX_PLY]; // [ply][move index]
	int pvlen[MAX_PLY]; // [ply] length of the principal variation at that ply

	void clear() {
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 64; j++) {
				for (int k = 0; k < 64; k++) {
					history[i][j][k] = 0;
					cmh[i][j][k] = NullMove;
				}
			}
			for (int j = 0; j < 6; j++) {
				for (int k = 0; k < 6; k++) {
					for (int l = 0; l < 64; l++) {
						capthist[j][k][l] = 0;
					}
				}
			}
			for (int j = 0; j < CORRHIST_SZ; j++) {
				corrhist_ps[i][j] = corrhist_mat[i][j] = 0;
			}
		}
		for (int i = 0; i < MAX_PLY; i++) {
			pvlen[i] = 0;
			for (int j = 0; j < MAX_PLY; j++) {
				pvtable[i][j] = NullMove;
			}
		}
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < MAX_PLY; j++) {
				killer[i][j] = NullMove;
			}
		}
	}

	SearchParams() {
		clear();
	}
};

std::pair<Move, Value> search(Board &board, SearchParams &params, BoardState &bs, int64_t time = 1e9, bool quiet = false);

std::pair<Move, Value> search_nodes(Board &board, SearchParams &params, uint64_t nodes, BoardState &bs);

uint64_t perft(Board &board, int depth);
