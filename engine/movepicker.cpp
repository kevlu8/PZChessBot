#include "movepicker.hpp"

MovePicker::MovePicker(Board &board, SSEntry *ss, int ply, History *main_hist, TTable::TTEntry *tentry) : board(board), ss(ss), ply(ply), main_hist(main_hist) {
	stage = MP_STAGE_TT;
	ttMove = tentry ? tentry->best_move : NullMove;
}

Move MovePicker::next() {
	if (stage == MP_STAGE_DONE)
		return NullMove;

	if (stage == MP_STAGE_TT) {
		stage = MP_STAGE_GEN;
		if (ttMove != NullMove && board.is_pseudolegal(ttMove))
			return ttMove;
	}

	if (stage == MP_STAGE_GEN) {
		stage = MP_STAGE_GOODNOISY;
		board.legal_moves(moves);

		const int CAPTURE_PROMO_BASE = 1000000;
		const int QUIET_BASE = 0;

		for (Move move : moves) {
			if (move == ttMove)
				continue;

			int score = 0;

			bool capt = board.is_capture(move);
			bool promo = move.type() == PROMOTION;

			if (capt || promo) {
				score = CAPTURE_PROMO_BASE;
				if (capt)
					score += PieceValue[board.mailbox[move.dst()] & 7] +
							 main_hist->capthist[board.mailbox[move.src()] & 7][board.mailbox[move.dst()] & 7][move.dst()];
				if (promo)
					score += PieceValue[move.promotion() + KNIGHT] - PawnValue;
				
				if (board.see_capture(move) >= -100) {
					scores_goodnoisy.push_back({move, score});
				} else {
					scores_badnoisy.push_back({move, score});
				}
			} else {
				if (qskip) continue;
				score = QUIET_BASE + main_hist->get_history(board, move, ply, ss);
				if (move == ss->killer) score += 1457;
				scores_quiet.push_back({move, score});
			}
		}

		end = scores_goodnoisy.size();
	}

	if (stage == MP_STAGE_GOODNOISY) {
		if (end == 0) {
			stage = MP_STAGE_QUIET;
			end = scores_quiet.size();
		} else {
			Move best_move = NullMove;
			int best_score = -2147483647;
			int idx = 0;
			for (int i = 0; i < end; i++) {
				if (scores_goodnoisy[i].second > best_score) {
					best_score = scores_goodnoisy[i].second;
					best_move = scores_goodnoisy[i].first;
					idx = i;
				}
			}
			std::swap(scores_goodnoisy[idx], scores_goodnoisy[end - 1]);
			end--;
			return best_move;
		}
	}

	if (stage == MP_STAGE_QUIET) {
		if (end == 0 || qskip) {
			stage = MP_STAGE_BADNOISY;
			end = scores_badnoisy.size();
		} else {
			Move best_move = NullMove;
			int best_score = -2147483647;
			int idx = 0;
			for (int i = 0; i < end; i++) {
				if (scores_quiet[i].second > best_score) {
					best_score = scores_quiet[i].second;
					best_move = scores_quiet[i].first;
					idx = i;
				}
			}
			std::swap(scores_quiet[idx], scores_quiet[end - 1]);
			end--;
			return best_move;
		}
	}

	if (stage == MP_STAGE_BADNOISY) {
		if (end == 0) {
			stage = MP_STAGE_DONE;
			return NullMove;
		}
		Move best_move = NullMove;
		int best_score = -2147483647;
		int idx = 0;
		for (int i = 0; i < end; i++) {
			if (scores_badnoisy[i].second > best_score) {
				best_score = scores_badnoisy[i].second;
				best_move = scores_badnoisy[i].first;
				idx = i;
			}
		}
		std::swap(scores_badnoisy[idx], scores_badnoisy[end - 1]);
		end--;
		if (end == 0) stage = MP_STAGE_DONE;
		return best_move;
	}

	if (stage == MP_STAGE_DONE) {
		return NullMove;
	}

	std::cout << "Invalid MovePicker stage: " << stage << std::endl;
	throw std::runtime_error("Il faut que vous mouriez");
}
