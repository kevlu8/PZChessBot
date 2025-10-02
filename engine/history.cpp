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
void History::update_corrhist(bool side, uint64_t pshash, uint64_t mathash, uint64_t nphash, Move prev, Value diff, int depth) {
	const Value sdiff = diff * CORRHIST_GRAIN;
	const Value weight = std::min(depth * depth, 128);
	Value &pscorr = corrhist_ps[side][pshash % CORRHIST_SZ];
	pscorr = std::clamp((pscorr * (CORRHIST_WEIGHT - weight) + sdiff * weight) / CORRHIST_WEIGHT, -MAX_HISTORY, (int)MAX_HISTORY);
	Value &matcorr = corrhist_mat[side][mathash % CORRHIST_SZ];
	matcorr = std::clamp((matcorr * (CORRHIST_WEIGHT - weight) + sdiff * weight) / CORRHIST_WEIGHT, -MAX_HISTORY, (int)MAX_HISTORY);
	Value &movcorr = corrhist_prev[side][prev.src()][prev.dst()];
	movcorr = std::clamp((movcorr * (CORRHIST_WEIGHT - weight) + sdiff * weight) / CORRHIST_WEIGHT, -MAX_HISTORY, (int)MAX_HISTORY);
	Value &npcorr = corrhist_np[side][nphash % CORRHIST_SZ];
	npcorr = std::clamp((npcorr * (CORRHIST_WEIGHT - weight) + sdiff * weight) / CORRHIST_WEIGHT, -MAX_HISTORY, (int)MAX_HISTORY);
}

void History::apply_correction(bool side, uint64_t pshash, uint64_t mathash, uint64_t nphash, Move prev, Value &eval) {
	if (abs(eval) >= VALUE_MATE_MAX_PLY)
		return; // Don't apply correction if we are already at a mate score
	const Value pscorr = corrhist_ps[side][pshash % CORRHIST_SZ];
	const Value matcorr = corrhist_mat[side][mathash % CORRHIST_SZ];
	const Value movcorr = corrhist_prev[side][prev.src()][prev.dst()];
	const Value npcorr = corrhist_np[side][nphash % CORRHIST_SZ];
	const Value corr = (pscorr + matcorr + movcorr + npcorr) / 2;
	eval = std::clamp(eval + corr / CORRHIST_GRAIN, -VALUE_MATE_MAX_PLY + 1, VALUE_MATE_MAX_PLY - 1);
}