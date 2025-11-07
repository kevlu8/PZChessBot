#include "history.hpp"

int History::get_conthist(Board &board, Move move, int ply, SSEntry *line) {
	int score = 0;
	if (ply >= 1 && (line - 1)->cont_hist)
		score += (line - 1)->cont_hist->hist[board.side][board.mailbox[move.src()] & 7][move.dst()];
	if (ply >= 2 && (line - 2)->cont_hist)
		score += (line - 2)->cont_hist->hist[board.side][board.mailbox[move.src()] & 7][move.dst()];
	return score;
}

int History::get_history(Board &board, Move move, int ply, SSEntry *line) {
	int score = history[board.side][move.src()][move.dst()];
	score += get_conthist(board, move, ply, line);
	return score;
}

int History::get_capthist(Board &board, Move move) {
	int score = capthist[board.mailbox[move.src()] & 7][board.mailbox[move.dst()] & 7][move.dst()];
	return score;
}

// History gravity formula
void History::update_history(Board &board, Move &move, int ply, SSEntry *line, Value bonus) {
	int cbonus = std::clamp(bonus, (Value)(-MAX_HISTORY), MAX_HISTORY);
	history[board.side][move.src()][move.dst()] += cbonus - history[board.side][move.src()][move.dst()] * abs(bonus) / MAX_HISTORY;
	int conthist = get_conthist(board, move, ply, line);
	if (ply >= 1 && (line - 1)->cont_hist)
		(line - 1)->cont_hist->hist[board.side][board.mailbox[move.src()] & 7][move.dst()] += cbonus - conthist * abs(bonus) / MAX_HISTORY;
	if (ply >= 2 && (line - 2)->cont_hist)
		(line - 2)->cont_hist->hist[board.side][board.mailbox[move.src()] & 7][move.dst()] += cbonus - conthist * abs(bonus) / MAX_HISTORY;
}

void History::update_capthist(PieceType piece, PieceType captured, Square dst, Value bonus) {
	int cbonus = std::clamp(bonus, (Value)(-MAX_HISTORY), MAX_HISTORY);
	capthist[piece][captured][dst] += cbonus - capthist[piece][captured][dst] * abs(bonus) / MAX_HISTORY;
}

// Moving exponential average for corrhist
void History::update_corrhist(Board &board, Value diff, int depth) {
	const Value sdiff = diff * CORRHIST_GRAIN;
	const Value weight = std::min(depth + 1, 16);
	
	auto update_entry = [=](Value &entry) {
		int update = entry * (CORRHIST_WEIGHT - weight) + sdiff * weight;
		entry = std::clamp(update / CORRHIST_WEIGHT, -MAX_HISTORY, (int)MAX_HISTORY);
	};

	update_entry(corrhist_ps[board.side][board.pawn_struct_hash() % CORRHIST_SZ]);
}

void History::apply_correction(Board &board, Value &eval) {
	if (abs(eval) >= VALUE_MATE_MAX_PLY)
		return; // Don't apply correction if we are already at a mate score
	
	int corr = 0;
	corr += corrhist_ps[board.side][board.pawn_struct_hash() % CORRHIST_SZ];
	
	eval += corr / CORRHIST_GRAIN;
}
