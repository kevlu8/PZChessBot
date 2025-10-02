#pragma once

#include "includes.hpp"

#include "pzstl/vector.hpp"

#include "bitboard.hpp"
#include "history.hpp"
#include "move.hpp"
#include "ttable.hpp"

enum MP_STAGE {
	MP_STAGE_TT,
	MP_STAGE_KILLER1,
	MP_STAGE_KILLER2,
	MP_STAGE_GEN,
	MP_STAGE_SCORE,
	MP_STAGE_MOVES,
	MP_STAGE_DONE,
};

class MovePicker {
private:
	Board &board;
	pzstd::vector<Move> *moves;
	pzstd::vector<std::pair<Move, int>> *scores;
	Move ttMove;
	SSEntry *ss;
	History *main_hist;
	int ply, end;

	MP_STAGE stage;
	bool qskip = false;

public:
	MovePicker(Board &board, SSEntry *ss, int ply, History *main_hist, TTable::TTEntry *tentry);

	Move next();

	void skip_quiets() {
		qskip = true;
	}
};
