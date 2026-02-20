#pragma once

#include "includes.hpp"

#include "pzstl/vector.hpp"

#include "bitboard.hpp"
#include "history.hpp"
#include "move.hpp"
#include "ttable.hpp"

enum MP_Stage {
	MP_STAGE_TT,
	MP_STAGE_GEN,
	MP_STAGE_GOODNOISY,
	MP_STAGE_MOVES,
	MP_STAGE_BADNOISY,

	MP_PC_TT,
	MP_PC_GEN,
	MP_PC_MOVES,

	MP_STAGE_DONE,
};

class MovePicker {
private:
	Board &board;
	pzstd::vector<Move> moves, bad_noisy;
	pzstd::vector<std::pair<Move, int>> quiet_scores, noisy_scores;
	Move ttMove;
	SSEntry *ss;
	History *main_hist;
	int ply, end;

	MP_Stage stage;
	bool qskip = false;

public:
	MovePicker(Board &board, SSEntry *ss, int ply, History *main_hist, std::optional<TTable::TTEntry> &tentry);
	MovePicker(Board &board, History *main_hist, std::optional<TTable::TTEntry> &tentry); // Probcut constructor
	MovePicker(Board &board, History *main_hist, bool skip_quiets); // QSearch constructor

	Move next();

	void skip_quiets() {
		qskip = true;
	}
};
