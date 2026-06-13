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

	MP_QS_GEN,
	MP_QS_MOVES,

	MP_STAGE_DONE,
};

class MovePicker {
private:
	Position &pos;
	pzstd::vector<Move> moves, bad_noisy;
	pzstd::vector<std::pair<Move, int>> quiet_scores, noisy_scores;
	Move ttMove;
	SSEntry *ss;
	History *main_hist;
	int ply, end;

	MP_Stage stage;
	bool qskip = false;

public:
	MovePicker(Position &pos, SSEntry *ss, int ply, History *main_hist, std::optional<TTable::TTEntry> &tentry);
	MovePicker(Position &pos, History *main_hist, std::optional<TTable::TTEntry> &tentry); // Probcut constructor
	MovePicker(Position &pos, History *main_hist, bool skip_quiets); // QSearch constructor

	Move next();

	void skip_quiets() {
		qskip = true;
	}
};
