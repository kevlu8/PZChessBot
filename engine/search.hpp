#pragma once

#include "bitboard.hpp"
#include "eval.hpp"
#include "movegen.hpp"
#include "ttable.hpp"
#include <algorithm>

struct SearchSettings {
	int RFP_THRESHOLD = 131;
	int RFP_IMPROVING = 30;
	int ASPIRATION_WINDOW = 32;
	int DELTA_THRESHOLD = 300;
	int FUTILITY_THRESHOLD = 304;
	int FUTILITY_THRESHOLD2 = 692;
	int RAZOR_MARGIN = 241;
	int HISTORY_MARGIN = 4000;
	int CMH_BONUS = 1021;
	int KILLER1 = 1500;
	int KILLER2 = 800;

	void change(const std::string &param, const std::string &value) {
		if (param == "RFP_THRESHOLD") RFP_THRESHOLD = std::stoi(value);
		else if (param == "RFP_IMPROVING") RFP_IMPROVING = std::stoi(value);
		else if (param == "ASPIRATION_WINDOW") ASPIRATION_WINDOW = std::stoi(value);
		else if (param == "DELTA_THRESHOLD") DELTA_THRESHOLD = std::stoi(value);
		else if (param == "FUTILITY_THRESHOLD") FUTILITY_THRESHOLD = std::stoi(value);
		else if (param == "FUTILITY_THRESHOLD2") FUTILITY_THRESHOLD2 = std::stoi(value);
		else if (param == "RAZOR_MARGIN") RAZOR_MARGIN = std::stoi(value);
		else if (param == "HISTORY_MARGIN") HISTORY_MARGIN = std::stoi(value);
		else if (param == "CMH_BONUS") CMH_BONUS = std::stoi(value);
		else if (param == "KILLER1") KILLER1 = std::stoi(value);
		else if (param == "KILLER2") KILLER2 = std::stoi(value);
	}
};

extern SearchSettings ss;

// // Eval per ply threshold for RFP
// // RFP stops searching if our position is so good that
// // we can afford to lose RFP_THRESHOLD eval units per ply
// // and still be in a better position. The lower the value,
// // the more aggressive RFP is.
// #define RFP_THRESHOLD (131 * CP_SCALE_FACTOR)
// #define RFP_IMPROVING (30 * CP_SCALE_FACTOR)

// // Aspiration window size(s)
// // The aspiration window is the range of values we search
// // for the best move. If we fail to find the best move in
// // this range, we expand the window.
// #define ASPIRATION_WINDOW (32 * CP_SCALE_FACTOR)

// Null-move pruning reduction value
// This is the amount of depth we reduce the search by
// when we do a null-move search
#define NMP_R_VALUE 4

// // Delta pruning threshold
// // This is the threshold for delta pruning (in centipawns)
// #define DELTA_THRESHOLD (300 * CP_SCALE_FACTOR)

// // Futility pruning threshold
// // This is the threshold for futility pruning (in centipawns)
// #define FUTILITY_THRESHOLD (304 * CP_SCALE_FACTOR)
// #define FUTILITY_THRESHOLD2 (692 * CP_SCALE_FACTOR)

// // Razoring margin
// // This is the margin for razoring (in centipawns)
// #define RAZOR_MARGIN (241 * CP_SCALE_FACTOR)

// // History pruning margin
// // This is the margin for history pruning (in centipawns)
// #define HISTORY_MARGIN (4000 * CP_SCALE_FACTOR)

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

std::pair<Move, Value> search(Board &board, int64_t time = 1e9, bool quiet = false);

std::pair<Move, Value> search_depth(Board &board, int depth, bool quiet = false);

std::pair<Move, Value> search_nodes(Board &board, uint64_t nodes, bool quiet = false);

uint64_t perft(Board &board, int depth);
