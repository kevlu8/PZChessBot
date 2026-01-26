#include "history.hpp"

int History::get_conthist(Board &board, Move move, int ply, SSEntry *line) {
	int score = 0;
	if (ply >= 1 && (line - 1)->cont_hist)
		score += (line - 1)->cont_hist->hist[board.side][board.mailbox[move.src()] & 7][move.dst()];
	if (ply >= 2 && (line - 2)->cont_hist)
		score += (line - 2)->cont_hist->hist[board.side][board.mailbox[move.src()] & 7][move.dst()];
	if (ply >= 3 && (line - 3)->cont_hist)
		score += (line - 3)->cont_hist->hist[board.side][board.mailbox[move.src()] & 7][move.dst()];
	if (ply >= 4 && (line - 4)->cont_hist)
		score += (line - 4)->cont_hist->hist[board.side][board.mailbox[move.src()] & 7][move.dst()];
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
	if (ply >= 3 && (line - 3)->cont_hist)
		(line - 3)->cont_hist->hist[board.side][board.mailbox[move.src()] & 7][move.dst()] += cbonus - conthist * abs(bonus) / MAX_HISTORY;
	if (ply >= 4 && (line - 4)->cont_hist)
		(line - 4)->cont_hist->hist[board.side][board.mailbox[move.src()] & 7][move.dst()] += cbonus - conthist * abs(bonus) / MAX_HISTORY;
}

void History::update_capthist(PieceType piece, PieceType captured, Square dst, Value bonus) {
	int cbonus = std::clamp(bonus, (Value)(-MAX_HISTORY), MAX_HISTORY);
	capthist[piece][captured][dst] += cbonus - capthist[piece][captured][dst] * abs(bonus) / MAX_HISTORY;
}

// Moving exponential average for corrhist
void History::update_corrhist(Board &board, SSEntry *line, int ply, int bonus) {
	auto update_entry = [=](Value &entry) {
		int update = std::clamp(bonus, -MAX_CORRHIST / 4, MAX_CORRHIST / 4);
		entry += update - entry * abs(update) / MAX_CORRHIST;
	};

	update_entry(corrhist_ps[board.side][board.pawn_hash() % CORRHIST_SZ]);
	update_entry(corrhist_np[board.side][WHITE][board.nonpawn_hash(WHITE) % CORRHIST_SZ]);
	update_entry(corrhist_np[board.side][BLACK][board.nonpawn_hash(BLACK) % CORRHIST_SZ]);
	update_entry(corrhist_maj[board.side][board.major_hash() % CORRHIST_SZ]);
	update_entry(corrhist_min[board.side][board.minor_hash() % CORRHIST_SZ]);
	if (ply >= 2)
		update_entry((line - 1)->corr_hist->hist[board.side][(line - 2)->piece][(line - 2)->move.dst()]);
}

void History::apply_correction(Board &board, SSEntry *line, int ply, Value &eval) {
	if (abs(eval) >= VALUE_MATE_MAX_PLY)
		return; // Don't apply correction if we are already at a mate score
	
	int corr = 0;
	corr += 117 * corrhist_ps[board.side][board.pawn_hash() % CORRHIST_SZ];
	corr += 134 * corrhist_np[board.side][WHITE][board.nonpawn_hash(WHITE) % CORRHIST_SZ];
	corr += 134 * corrhist_np[board.side][BLACK][board.nonpawn_hash(BLACK) % CORRHIST_SZ];
	corr += 61 * corrhist_maj[board.side][board.major_hash() % CORRHIST_SZ];
	corr += 67 * corrhist_min[board.side][board.minor_hash() % CORRHIST_SZ];
	if (ply >= 2)
		corr += 140 * (line - 1)->corr_hist->hist[board.side][(line - 2)->piece][(line - 2)->move.dst()];

	eval += corr / 2048;
}
