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

// Correction history table size
#define CORRHIST_SZ 16384
#define THREAT_PRIME_MOD 16381

struct ContHistEntry {
	Value hist[2][7][64]; // [side][piecetype][to]

	ContHistEntry() {
		memset(hist, 0, sizeof(hist));
	}
};

struct SSEntry {
	Move move;
	Value eval;
	Move excl;
	ContHistEntry *cont_hist, *corr_hist;
	Move killer = NullMove;
	PieceType piece = NO_PIECETYPE, captured = NO_PIECETYPE;
	int cutoffcnt = 0;

	SSEntry() : move(NullMove), eval(VALUE_NONE), excl(NullMove), cont_hist(nullptr), corr_hist(nullptr), cutoffcnt(0) {}
};

struct History {
	/**
	 * The history heuristic is a move ordering heuristic that helps sort quiet moves.
	 * It works by storing its effectiveness in the past through beta cutoffs.
	 * We store a history table for each side indexed by [src][dst].
	 */
	Value history[2][64][64][2][2]; // [side][src][dst][src_threatened][dst_threatened]
	ContHistEntry cont_hist[2][7][64]; // [side][piece][to]

	/**
	 * Capture history is a heuristic similar to the history heuristic, but it's used for
	 * captures. It basically replaces LVA.
	 */
	Value capthist[2][6][6][64]; // [piece][captured][dst]

	History() {
		memset(history, 0, sizeof(history));
		memset(cont_hist, 0, sizeof(cont_hist));
		memset(capthist, 0, sizeof(capthist));
	}

	int get_conthist(Position &pos, Move move, int ply, SSEntry *line);
	int get_history(Position &pos, Move move, int ply, SSEntry *line);
	int get_capthist(Position &pos, Move move);

	void update_history(Position &pos, Move &move, int ply, SSEntry *line, Value bonus);
	void update_conthist(Position &pos, Move move, int ply, SSEntry *line, Value bonus);
	void update_capthist(Position &pos, Move move, Value bonus);
};

struct Corrhist {
	/**
	 * Static Evaluation Correction History (CorrHist) is a heuristic that "corrects"
	 * the static evaluation towards the actual evaluation of the position, based on
	 * a variety of factors (e.g. pawn structure, minor pieces, etc)
	 */
	Value corrhist_ps[2][CORRHIST_SZ]; // [side][pawn hash]
	Value corrhist_np[2][2][CORRHIST_SZ]; // [side][color][color non-pawn hash]
	Value corrhist_maj[2][CORRHIST_SZ]; // [side][major piece hash]
	Value corrhist_min[2][CORRHIST_SZ]; // [side][minor piece hash]
	ContHistEntry corrhist_cont[2][7][64]; // [side][piece][to]
	Value corrhist_threat[2][CORRHIST_SZ];

	Corrhist() {
		memset(corrhist_ps, 0, sizeof(corrhist_ps));
		memset(corrhist_np, 0, sizeof(corrhist_np));
		memset(corrhist_maj, 0, sizeof(corrhist_maj));
		memset(corrhist_min, 0, sizeof(corrhist_min));
		memset(corrhist_cont, 0, sizeof(corrhist_cont));
		memset(corrhist_threat, 0, sizeof(corrhist_threat));
	}

	void update_corrhist(Position &pos, SSEntry *line, int ply, int bonus);
	void apply_correction(Position &pos, SSEntry *line, int ply, Value &eval);
};
