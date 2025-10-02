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
		stage = MP_STAGE_MOVES;
		board.legal_moves(moves);

		const int CAPTURE_PROMO_BASE = 1000000;
		const int KILLER_BASE = 800000;
		const int QUIET_BASE = 0;

		for (Move move : moves) {
			if (move == ttMove || move == ss->killer[0] || move == ss->killer[1])
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
			} else {
				if (qskip) continue;
				score = QUIET_BASE + main_hist->get_history(board, move, ply, ss);
				if (move == ss->killer[0]) score += KILLER_BASE + 1500;
				else if (move == ss->killer[1]) score += KILLER_BASE + 1200;
			}
			scores.push_back({move, score});
		}

		end = scores.size();
	}

	if (stage == MP_STAGE_MOVES) {
		Move best_move = NullMove;
		int best_score = -2147483647;
		int idx = 0;
		for (int i = 0; i < end; i++) {
			if (board.is_capture(scores[i].first) && qskip) continue;
			if (scores[i].second > best_score) {
				best_score = scores[i].second;
				best_move = scores[i].first;
				idx = i;
			}
		}
		std::swap(scores[idx], scores[end - 1]);
		end--;
		if (end == 0) stage = MP_STAGE_DONE;
		return best_move;
	}

	if (stage == MP_STAGE_DONE) {
		return NullMove;
	}

	std::cout << "Invalid MovePicker stage: " << stage << std::endl;
	throw std::runtime_error("Il faut que nous mourions");
}
