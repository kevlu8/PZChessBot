#pragma once

#include "includes.hpp"

#include "bitboard.hpp"

// Correction history table size
#define CORRHIST_SZ 16384

struct ContHistEntry {
	Value hist[2][6][64]; // [side][piecetype][to]

	ContHistEntry() {
		memset(hist, 0, sizeof(hist));
	}
};

struct SSEntry {
	Move move;
	Value eval;
	Move excl;
	ContHistEntry *cont_hist;
	Move killer = NullMove;

	SSEntry() : move(NullMove), eval(VALUE_NONE), excl(NullMove), cont_hist(nullptr) {}
};

class History {
public:
	/**
	 * The history heuristic is a move ordering heuristic that helps sort quiet moves.
	 * It works by storing its effectiveness in the past through beta cutoffs.
	 * We store a history table for each side indexed by [src][dst].
	 */
	Value history[2][64][64];
	ContHistEntry cont_hist[2][7][64]; // [side][piece][to]

	/**
	 * Capture history is a heuristic similar to the history heuristic, but it's used for
	 * captures. It basically replaces LVA.
	 */
	Value capthist[6][6][64]; // [piece][captured][dst]

	/**
	 * Static Evaluation Correction History (CorrHist) is a heuristic that "corrects"
	 * the static evaluation towards the actual evaluation of the position, based on
	 * a variety of factors (e.g. pawn structure, minor pieces, etc)
	 */
	Value corrhist_ps[2][CORRHIST_SZ]; // [side][pawn hash]
	Value corrhist_np[2][2][CORRHIST_SZ]; // [side][color][color non-pawn hash]
	Value corrhist_maj[2][CORRHIST_SZ]; // [side][major piece hash]
	Value corrhist_min[2][CORRHIST_SZ]; // [side][minor piece hash]

	History() {
		memset(history, 0, sizeof(history));
		memset(capthist, 0, sizeof(capthist));
		memset(corrhist_ps, 0, sizeof(corrhist_ps));
		memset(corrhist_np, 0, sizeof(corrhist_np));
		memset(corrhist_maj, 0, sizeof(corrhist_maj));
		memset(corrhist_min, 0, sizeof(corrhist_min));
	}

	int get_conthist(Board &board, Move move, int ply, SSEntry *line);
	int get_history(Board &board, Move move, int ply, SSEntry *line);
	int get_capthist(Board &board, Move move);

	void update_history(Board &board, Move &move, int ply, SSEntry *line, Value bonus);
	void update_capthist(PieceType piece, PieceType captured, Square dst, Value bonus);

	void update_corrhist(Board &board, int bonus);
	void apply_correction(Board &board, Value &eval);
};
