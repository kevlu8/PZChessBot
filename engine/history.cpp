#include "history.hpp"
#include "params.hpp"

int History::get_conthist(Position &pos, Move move, int ply, SSEntry *line) {
	int score = 0;
	if (ply >= 1 && (line - 1)->cont_hist)
		score += (line - 1)->cont_hist->hist[pos.side][pos.mailbox[move.src()] & 7][move.dst()];
	if (ply >= 2 && (line - 2)->cont_hist)
		score += (line - 2)->cont_hist->hist[pos.side][pos.mailbox[move.src()] & 7][move.dst()];
	if (ply >= 3 && (line - 3)->cont_hist)
		score += (line - 3)->cont_hist->hist[pos.side][pos.mailbox[move.src()] & 7][move.dst()];
	if (ply >= 4 && (line - 4)->cont_hist)
		score += (line - 4)->cont_hist->hist[pos.side][pos.mailbox[move.src()] & 7][move.dst()];
	if (ply >= 6 && (line - 6)->cont_hist)
		score += (line - 6)->cont_hist->hist[pos.side][pos.mailbox[move.src()] & 7][move.dst()];
	return score;
}

int History::get_history(Position &pos, Move move, int ply, SSEntry *line) {
	int score = history[pos.side][move.src()][move.dst()][pos.control(move.src(), !pos.side)][pos.control(move.dst(), !pos.side)];
	score += get_conthist(pos, move, ply, line);
	return score;
}

int History::get_capthist(Position &pos, Move move) {
	int score = capthist[pos.side][pos.mailbox[move.src()] & 7][pos.mailbox[move.dst()] & 7][move.dst()];
	return score;
}

// History gravity formula
void History::update_history(Position &pos, Move &move, int ply, SSEntry *line, Value bonus) {
	int cbonus = std::clamp(bonus, (Value)(-MAX_HISTORY), MAX_HISTORY);
	history[pos.side][move.src()][move.dst()][pos.control(move.src(), !pos.side)][pos.control(move.dst(), !pos.side)] += cbonus - history[pos.side][move.src()][move.dst()][pos.control(move.src(), !pos.side)][pos.control(move.dst(), !pos.side)] * abs(bonus) / MAX_HISTORY;
	update_conthist(pos, move, ply, line, bonus);
}

void History::update_conthist(Position &pos, Move move, int ply, SSEntry *line, Value bonus) {
	int cbonus = std::clamp(bonus, (Value)(-MAX_HISTORY), MAX_HISTORY);
	int conthist = get_conthist(pos, move, ply, line);
	if (ply >= 1 && (line - 1)->cont_hist)
		(line - 1)->cont_hist->hist[pos.side][pos.mailbox[move.src()] & 7][move.dst()] += cbonus - conthist * abs(bonus) / MAX_HISTORY;
	if (ply >= 2 && (line - 2)->cont_hist)
		(line - 2)->cont_hist->hist[pos.side][pos.mailbox[move.src()] & 7][move.dst()] += cbonus - conthist * abs(bonus) / MAX_HISTORY;
	if (ply >= 3 && (line - 3)->cont_hist)
		(line - 3)->cont_hist->hist[pos.side][pos.mailbox[move.src()] & 7][move.dst()] += cbonus - conthist * abs(bonus) / MAX_HISTORY;
	if (ply >= 4 && (line - 4)->cont_hist)
		(line - 4)->cont_hist->hist[pos.side][pos.mailbox[move.src()] & 7][move.dst()] += cbonus - conthist * abs(bonus) / MAX_HISTORY;
	if (ply >= 6 && (line - 6)->cont_hist)
		(line - 6)->cont_hist->hist[pos.side][pos.mailbox[move.src()] & 7][move.dst()] += cbonus - conthist * abs(bonus) / MAX_HISTORY;
}

void History::update_capthist(Position &pos, Move move, Value bonus) {
	int cbonus = std::clamp(bonus, (Value)(-MAX_HISTORY), MAX_HISTORY);
	capthist[pos.side][pos.mailbox[move.src()] & 7][pos.mailbox[move.dst()] & 7][move.dst()] += cbonus - capthist[pos.side][pos.mailbox[move.src()] & 7][pos.mailbox[move.dst()] & 7][move.dst()] * abs(bonus) / MAX_HISTORY;
}

// Moving exponential average for corrhist
void Corrhist::update_corrhist(Position &pos, SSEntry *line, int ply, int bonus) {
	auto update_entry = [=](Value &entry) {
		int update = std::clamp(bonus, -MAX_CORRHIST / 4, MAX_CORRHIST / 4);
		entry += update - entry * abs(update) / MAX_CORRHIST;
	};

	update_entry(corrhist_ps[pos.side][pos.pawn_hash() % CORRHIST_SZ]);
	update_entry(corrhist_np[pos.side][WHITE][pos.nonpawn_hash(WHITE) % CORRHIST_SZ]);
	update_entry(corrhist_np[pos.side][BLACK][pos.nonpawn_hash(BLACK) % CORRHIST_SZ]);
	update_entry(corrhist_maj[pos.side][pos.major_hash() % CORRHIST_SZ]);
	update_entry(corrhist_min[pos.side][pos.minor_hash() % CORRHIST_SZ]);
	if (ply >= 2)
		update_entry((line - 1)->corr_hist->hist[pos.side][(line - 2)->piece][(line - 2)->move.dst()]);
	if (ply >= 3)
		update_entry((line - 1)->corr_hist->hist[pos.side][(line - 3)->piece][(line - 3)->move.dst()]);
	update_entry(corrhist_threat[pos.side][(pos.side_control[pos.side]) % THREAT_PRIME_MOD]);
}

void Corrhist::apply_correction(Position &pos, SSEntry *line, int ply, Value &eval) {
	if (abs(eval) >= VALUE_WIN)
		return; // Don't apply correction if we are already at a mate score
	
	int corr = 0;
	corr += corr_ps() * corrhist_ps[pos.side][pos.pawn_hash() % CORRHIST_SZ];
	corr += corr_np() * corrhist_np[pos.side][WHITE][pos.nonpawn_hash(WHITE) % CORRHIST_SZ];
	corr += corr_np() * corrhist_np[pos.side][BLACK][pos.nonpawn_hash(BLACK) % CORRHIST_SZ];
	corr += corr_maj() * corrhist_maj[pos.side][pos.major_hash() % CORRHIST_SZ];
	corr += corr_min() * corrhist_min[pos.side][pos.minor_hash() % CORRHIST_SZ];
	if (ply >= 2)
		corr += corr_cont() * (line - 1)->corr_hist->hist[pos.side][(line - 2)->piece][(line - 2)->move.dst()];
	if (ply >= 3)
		corr += corr_cont2() * (line - 1)->corr_hist->hist[pos.side][(line - 3)->piece][(line - 3)->move.dst()];
	corr += corr_threat() * corrhist_threat[pos.side][(pos.side_control[pos.side]) % THREAT_PRIME_MOD];

	eval += corr / 2048;
}
