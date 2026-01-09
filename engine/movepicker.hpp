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
	MP_STAGE_QUIET,
	MP_STAGE_BADNOISY,
	MP_STAGE_DONE,
};

class MovePicker {
private:
	Board &board;
	pzstd::vector<Move> moves;
	pzstd::vector<std::pair<Move, int>> scores_quiet, scores_goodnoisy, scores_badnoisy;
	Move ttMove;
	SSEntry *ss;
	History *main_hist;
	int ply, end;

	MP_Stage stage;
	bool qskip = false;

public:
	MovePicker(Board &board, SSEntry *ss, int ply, History *main_hist, TTable::TTEntry *tentry);

	Move next();

	void skip_quiets() {
		qskip = true;
	}
};
